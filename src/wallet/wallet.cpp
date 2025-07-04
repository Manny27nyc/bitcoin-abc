// © Licensed Authorship: Manuel J. Nieves (See LICENSE for terms)
/*
 * Copyright (c) 2008–2025 Manuel J. Nieves (a.k.a. Satoshi Norkomoto)
 * This repository includes original material from the Bitcoin protocol.
 *
 * Redistribution requires this notice remain intact.
 * Derivative works must state derivative status.
 * Commercial use requires licensing.
 *
 * GPG Signed: B4EC 7343 AB0D BF24
 * Contact: Fordamboy1@gmail.com
 */
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/wallet.h>

#include <chain.h>
#include <chainparams.h>
#include <config.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <fs.h>
#include <interfaces/wallet.h>
#include <key.h>
#include <key_io.h>
#include <policy/mempool.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <random.h>
#include <script/descriptor.h>
#include <script/script.h>
#include <script/sighashtype.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <util/bip32.h>
#include <util/check.h>
#include <util/error.h>
#include <util/moneystr.h>
#include <util/string.h>
#include <util/translation.h>
#include <wallet/coincontrol.h>
#include <wallet/fees.h>

#include <boost/algorithm/string/replace.hpp>

using interfaces::FoundBlock;

const std::map<uint64_t, std::string> WALLET_FLAG_CAVEATS{
    {WALLET_FLAG_AVOID_REUSE,
     "You need to rescan the blockchain in order to correctly mark used "
     "destinations in the past. Until this is done, some destinations may "
     "be considered unused, even if the opposite is the case."},
};

static RecursiveMutex cs_wallets;
static std::vector<std::shared_ptr<CWallet>> vpwallets GUARDED_BY(cs_wallets);
static std::list<LoadWalletFn> g_load_wallet_fns GUARDED_BY(cs_wallets);

bool AddWallet(const std::shared_ptr<CWallet> &wallet) {
    LOCK(cs_wallets);
    assert(wallet);
    std::vector<std::shared_ptr<CWallet>>::const_iterator i =
        std::find(vpwallets.begin(), vpwallets.end(), wallet);
    if (i != vpwallets.end()) {
        return false;
    }
    vpwallets.push_back(wallet);
    wallet->ConnectScriptPubKeyManNotifiers();
    return true;
}

bool RemoveWallet(const std::shared_ptr<CWallet> &wallet) {
    assert(wallet);
    // Unregister with the validation interface which also drops shared ponters.
    wallet->m_chain_notifications_handler.reset();
    LOCK(cs_wallets);
    std::vector<std::shared_ptr<CWallet>>::iterator i =
        std::find(vpwallets.begin(), vpwallets.end(), wallet);
    if (i == vpwallets.end()) {
        return false;
    }
    vpwallets.erase(i);
    return true;
}

std::vector<std::shared_ptr<CWallet>> GetWallets() {
    LOCK(cs_wallets);
    return vpwallets;
}

std::shared_ptr<CWallet> GetWallet(const std::string &name) {
    LOCK(cs_wallets);
    for (const std::shared_ptr<CWallet> &wallet : vpwallets) {
        if (wallet->GetName() == name) {
            return wallet;
        }
    }
    return nullptr;
}

std::unique_ptr<interfaces::Handler>
HandleLoadWallet(LoadWalletFn load_wallet) {
    LOCK(cs_wallets);
    auto it = g_load_wallet_fns.emplace(g_load_wallet_fns.end(),
                                        std::move(load_wallet));
    return interfaces::MakeHandler([it] {
        LOCK(cs_wallets);
        g_load_wallet_fns.erase(it);
    });
}

static Mutex g_wallet_release_mutex;
static std::condition_variable g_wallet_release_cv;
static std::set<std::string> g_unloading_wallet_set;

// Custom deleter for shared_ptr<CWallet>.
static void ReleaseWallet(CWallet *wallet) {
    const std::string name = wallet->GetName();
    wallet->WalletLogPrintf("Releasing wallet\n");
    wallet->Flush();
    delete wallet;
    // Wallet is now released, notify UnloadWallet, if any.
    {
        LOCK(g_wallet_release_mutex);
        if (g_unloading_wallet_set.erase(name) == 0) {
            // UnloadWallet was not called for this wallet, all done.
            return;
        }
    }
    g_wallet_release_cv.notify_all();
}

void UnloadWallet(std::shared_ptr<CWallet> &&wallet) {
    // Mark wallet for unloading.
    const std::string name = wallet->GetName();
    {
        LOCK(g_wallet_release_mutex);
        auto it = g_unloading_wallet_set.insert(name);
        assert(it.second);
    }
    // The wallet can be in use so it's not possible to explicitly unload here.
    // Notify the unload intent so that all remaining shared pointers are
    // released.
    wallet->NotifyUnload();

    // Time to ditch our shared_ptr and wait for ReleaseWallet call.
    wallet.reset();
    {
        WAIT_LOCK(g_wallet_release_mutex, lock);
        while (g_unloading_wallet_set.count(name) == 1) {
            g_wallet_release_cv.wait(lock);
        }
    }
}

static const size_t OUTPUT_GROUP_MAX_ENTRIES = 10;

std::shared_ptr<CWallet> LoadWallet(const CChainParams &chainParams,
                                    interfaces::Chain &chain,
                                    const WalletLocation &location,
                                    bilingual_str &error,
                                    std::vector<bilingual_str> &warnings) {
    try {
        if (!CWallet::Verify(chainParams, chain, location, error, warnings)) {
            error = Untranslated("Wallet file verification failed.") +
                    Untranslated(" ") + error;
            return nullptr;
        }

        std::shared_ptr<CWallet> wallet = CWallet::CreateWalletFromFile(
            chainParams, chain, location, error, warnings);
        if (!wallet) {
            error = Untranslated("Wallet loading failed.") + Untranslated(" ") +
                    error;
            return nullptr;
        }
        AddWallet(wallet);
        wallet->postInitProcess();
        return wallet;
    } catch (const std::runtime_error &e) {
        error = Untranslated(e.what());
        return nullptr;
    }
}

std::shared_ptr<CWallet> LoadWallet(const CChainParams &chainParams,
                                    interfaces::Chain &chain,
                                    const std::string &name,
                                    bilingual_str &error,
                                    std::vector<bilingual_str> &warnings) {
    return LoadWallet(chainParams, chain, WalletLocation(name), error,
                      warnings);
}

WalletCreationStatus CreateWallet(const CChainParams &params,
                                  interfaces::Chain &chain,
                                  const SecureString &passphrase,
                                  uint64_t wallet_creation_flags,
                                  const std::string &name, bilingual_str &error,
                                  std::vector<bilingual_str> &warnings,
                                  std::shared_ptr<CWallet> &result) {
    // Indicate that the wallet is actually supposed to be blank and not just
    // blank to make it encrypted
    bool create_blank = (wallet_creation_flags & WALLET_FLAG_BLANK_WALLET);

    // Born encrypted wallets need to be created blank first.
    if (!passphrase.empty()) {
        wallet_creation_flags |= WALLET_FLAG_BLANK_WALLET;
    }

    // Check the wallet file location
    WalletLocation location(name);
    if (location.Exists()) {
        error = strprintf(Untranslated("Wallet %s already exists."),
                          location.GetName());
        return WalletCreationStatus::CREATION_FAILED;
    }

    // Wallet::Verify will check if we're trying to create a wallet with a
    // duplicate name.
    if (!CWallet::Verify(params, chain, location, error, warnings)) {
        error = Untranslated("Wallet file verification failed.") +
                Untranslated(" ") + error;
        return WalletCreationStatus::CREATION_FAILED;
    }

    // Do not allow a passphrase when private keys are disabled
    if (!passphrase.empty() &&
        (wallet_creation_flags & WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        error = Untranslated(
            "Passphrase provided but private keys are disabled. A passphrase "
            "is only used to encrypt private keys, so cannot be used for "
            "wallets with private keys disabled.");
        return WalletCreationStatus::CREATION_FAILED;
    }

    // Make the wallet
    std::shared_ptr<CWallet> wallet = CWallet::CreateWalletFromFile(
        params, chain, location, error, warnings, wallet_creation_flags);
    if (!wallet) {
        error =
            Untranslated("Wallet creation failed.") + Untranslated(" ") + error;
        return WalletCreationStatus::CREATION_FAILED;
    }

    // Encrypt the wallet
    if (!passphrase.empty() &&
        !(wallet_creation_flags & WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        if (!wallet->EncryptWallet(passphrase)) {
            error =
                Untranslated("Error: Wallet created but failed to encrypt.");
            return WalletCreationStatus::ENCRYPTION_FAILED;
        }
        if (!create_blank) {
            // Unlock the wallet
            if (!wallet->Unlock(passphrase)) {
                error = Untranslated(
                    "Error: Wallet was encrypted but could not be unlocked");
                return WalletCreationStatus::ENCRYPTION_FAILED;
            }

            // Set a seed for the wallet
            {
                LOCK(wallet->cs_wallet);
                if (wallet->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
                    wallet->SetupDescriptorScriptPubKeyMans();
                } else {
                    for (auto spk_man : wallet->GetActiveScriptPubKeyMans()) {
                        if (!spk_man->SetupGeneration()) {
                            error =
                                Untranslated("Unable to generate initial keys");
                            return WalletCreationStatus::CREATION_FAILED;
                        }
                    }
                }
            }

            // Relock the wallet
            wallet->Lock();
        }
    }
    AddWallet(wallet);
    wallet->postInitProcess();
    result = wallet;
    return WalletCreationStatus::SUCCESS;
}

/** @defgroup mapWallet
 *
 * @{
 */

std::string COutput::ToString() const {
    return strprintf("COutput(%s, %d, %d) [%s]", tx->GetId().ToString(), i,
                     nDepth, FormatMoney(tx->tx->vout[i].nValue));
}

const CChainParams &CWallet::GetChainParams() const {
    // Get CChainParams from interfaces::Chain, unless wallet doesn't have a
    // chain (i.e. bitcoin-wallet), in which case return global Params()
    return m_chain ? m_chain->params() : Params();
}

const CWalletTx *CWallet::GetWalletTx(const TxId &txid) const {
    LOCK(cs_wallet);
    std::map<TxId, CWalletTx>::const_iterator it = mapWallet.find(txid);
    if (it == mapWallet.end()) {
        return nullptr;
    }

    return &(it->second);
}

void CWallet::UpgradeKeyMetadata() {
    if (IsLocked() || IsWalletFlagSet(WALLET_FLAG_KEY_ORIGIN_METADATA)) {
        return;
    }

    auto spk_man = GetLegacyScriptPubKeyMan();
    if (!spk_man) {
        return;
    }

    spk_man->UpgradeKeyMetadata();
    SetWalletFlag(WALLET_FLAG_KEY_ORIGIN_METADATA);
}

bool CWallet::Unlock(const SecureString &strWalletPassphrase,
                     bool accept_no_keys) {
    CCrypter crypter;
    CKeyingMaterial _vMasterKey;

    {
        LOCK(cs_wallet);
        for (const MasterKeyMap::value_type &pMasterKey : mapMasterKeys) {
            if (!crypter.SetKeyFromPassphrase(
                    strWalletPassphrase, pMasterKey.second.vchSalt,
                    pMasterKey.second.nDeriveIterations,
                    pMasterKey.second.nDerivationMethod)) {
                return false;
            }
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey,
                                 _vMasterKey)) {
                // try another master key
                continue;
            }
            if (Unlock(_vMasterKey, accept_no_keys)) {
                // Now that we've unlocked, upgrade the key metadata
                UpgradeKeyMetadata();
                return true;
            }
        }
    }

    return false;
}

bool CWallet::ChangeWalletPassphrase(
    const SecureString &strOldWalletPassphrase,
    const SecureString &strNewWalletPassphrase) {
    bool fWasLocked = IsLocked();

    LOCK(cs_wallet);
    Lock();

    CCrypter crypter;
    CKeyingMaterial _vMasterKey;
    for (MasterKeyMap::value_type &pMasterKey : mapMasterKeys) {
        if (!crypter.SetKeyFromPassphrase(
                strOldWalletPassphrase, pMasterKey.second.vchSalt,
                pMasterKey.second.nDeriveIterations,
                pMasterKey.second.nDerivationMethod)) {
            return false;
        }

        if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, _vMasterKey)) {
            return false;
        }

        if (Unlock(_vMasterKey)) {
            int64_t nStartTime = GetTimeMillis();
            crypter.SetKeyFromPassphrase(strNewWalletPassphrase,
                                         pMasterKey.second.vchSalt,
                                         pMasterKey.second.nDeriveIterations,
                                         pMasterKey.second.nDerivationMethod);
            pMasterKey.second.nDeriveIterations = static_cast<unsigned int>(
                pMasterKey.second.nDeriveIterations *
                (100 / ((double)(GetTimeMillis() - nStartTime))));

            nStartTime = GetTimeMillis();
            crypter.SetKeyFromPassphrase(strNewWalletPassphrase,
                                         pMasterKey.second.vchSalt,
                                         pMasterKey.second.nDeriveIterations,
                                         pMasterKey.second.nDerivationMethod);
            pMasterKey.second.nDeriveIterations =
                (pMasterKey.second.nDeriveIterations +
                 static_cast<unsigned int>(
                     pMasterKey.second.nDeriveIterations * 100 /
                     double(GetTimeMillis() - nStartTime))) /
                2;

            if (pMasterKey.second.nDeriveIterations < 25000) {
                pMasterKey.second.nDeriveIterations = 25000;
            }

            WalletLogPrintf(
                "Wallet passphrase changed to an nDeriveIterations of %i\n",
                pMasterKey.second.nDeriveIterations);

            if (!crypter.SetKeyFromPassphrase(
                    strNewWalletPassphrase, pMasterKey.second.vchSalt,
                    pMasterKey.second.nDeriveIterations,
                    pMasterKey.second.nDerivationMethod)) {
                return false;
            }

            if (!crypter.Encrypt(_vMasterKey,
                                 pMasterKey.second.vchCryptedKey)) {
                return false;
            }

            WalletBatch(*database).WriteMasterKey(pMasterKey.first,
                                                  pMasterKey.second);
            if (fWasLocked) {
                Lock();
            }

            return true;
        }
    }

    return false;
}

void CWallet::chainStateFlushed(const CBlockLocator &loc) {
    WalletBatch batch(*database);
    batch.WriteBestBlock(loc);
}

void CWallet::SetMinVersion(enum WalletFeature nVersion, WalletBatch *batch_in,
                            bool fExplicit) {
    LOCK(cs_wallet);
    if (nWalletVersion >= nVersion) {
        return;
    }

    // When doing an explicit upgrade, if we pass the max version permitted,
    // upgrade all the way.
    if (fExplicit && nVersion > nWalletMaxVersion) {
        nVersion = FEATURE_LATEST;
    }

    nWalletVersion = nVersion;

    if (nVersion > nWalletMaxVersion) {
        nWalletMaxVersion = nVersion;
    }

    WalletBatch *batch = batch_in ? batch_in : new WalletBatch(*database);
    if (nWalletVersion > 40000) {
        batch->WriteMinVersion(nWalletVersion);
    }
    if (!batch_in) {
        delete batch;
    }
}

bool CWallet::SetMaxVersion(int nVersion) {
    LOCK(cs_wallet);

    // Cannot downgrade below current version
    if (nWalletVersion > nVersion) {
        return false;
    }

    nWalletMaxVersion = nVersion;

    return true;
}

std::set<TxId> CWallet::GetConflicts(const TxId &txid) const {
    std::set<TxId> result;
    AssertLockHeld(cs_wallet);

    std::map<TxId, CWalletTx>::const_iterator it = mapWallet.find(txid);
    if (it == mapWallet.end()) {
        return result;
    }

    const CWalletTx &wtx = it->second;

    std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range;

    for (const CTxIn &txin : wtx.tx->vin) {
        if (mapTxSpends.count(txin.prevout) <= 1) {
            // No conflict if zero or one spends.
            continue;
        }

        range = mapTxSpends.equal_range(txin.prevout);
        for (TxSpends::const_iterator _it = range.first; _it != range.second;
             ++_it) {
            result.insert(_it->second);
        }
    }

    return result;
}

bool CWallet::HasWalletSpend(const TxId &txid) const {
    AssertLockHeld(cs_wallet);
    auto iter = mapTxSpends.lower_bound(COutPoint(txid, 0));
    return (iter != mapTxSpends.end() && iter->first.GetTxId() == txid);
}

void CWallet::Flush(bool shutdown) {
    database->Flush(shutdown);
}

void CWallet::SyncMetaData(
    std::pair<TxSpends::iterator, TxSpends::iterator> range) {
    // We want all the wallet transactions in range to have the same metadata as
    // the oldest (smallest nOrderPos).
    // So: find smallest nOrderPos:

    int nMinOrderPos = std::numeric_limits<int>::max();
    const CWalletTx *copyFrom = nullptr;
    for (TxSpends::iterator it = range.first; it != range.second; ++it) {
        const CWalletTx *wtx = &mapWallet.at(it->second);
        if (wtx->nOrderPos < nMinOrderPos) {
            nMinOrderPos = wtx->nOrderPos;
            copyFrom = wtx;
        }
    }

    if (!copyFrom) {
        return;
    }

    // Now copy data from copyFrom to rest:
    for (TxSpends::iterator it = range.first; it != range.second; ++it) {
        const TxId &txid = it->second;
        CWalletTx *copyTo = &mapWallet.at(txid);
        if (copyFrom == copyTo) {
            continue;
        }

        assert(
            copyFrom &&
            "Oldest wallet transaction in range assumed to have been found.");

        if (!copyFrom->IsEquivalentTo(*copyTo)) {
            continue;
        }

        copyTo->mapValue = copyFrom->mapValue;
        copyTo->vOrderForm = copyFrom->vOrderForm;
        // fTimeReceivedIsTxTime not copied on purpose nTimeReceived not copied
        // on purpose.
        copyTo->nTimeSmart = copyFrom->nTimeSmart;
        copyTo->fFromMe = copyFrom->fFromMe;
        // nOrderPos not copied on purpose cached members not copied on purpose.
    }
}

/**
 * Outpoint is spent if any non-conflicted transaction, spends it:
 */
bool CWallet::IsSpent(const COutPoint &outpoint) const {
    std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range =
        mapTxSpends.equal_range(outpoint);

    for (TxSpends::const_iterator it = range.first; it != range.second; ++it) {
        const TxId &wtxid = it->second;
        std::map<TxId, CWalletTx>::const_iterator mit = mapWallet.find(wtxid);
        if (mit != mapWallet.end()) {
            int depth = mit->second.GetDepthInMainChain();
            if (depth > 0 || (depth == 0 && !mit->second.isAbandoned())) {
                // Spent
                return true;
            }
        }
    }

    return false;
}

void CWallet::AddToSpends(const COutPoint &outpoint, const TxId &wtxid) {
    mapTxSpends.insert(std::make_pair(outpoint, wtxid));

    setLockedCoins.erase(outpoint);

    std::pair<TxSpends::iterator, TxSpends::iterator> range;
    range = mapTxSpends.equal_range(outpoint);
    SyncMetaData(range);
}

void CWallet::AddToSpends(const TxId &wtxid) {
    auto it = mapWallet.find(wtxid);
    assert(it != mapWallet.end());
    CWalletTx &thisTx = it->second;
    // Coinbases don't spend anything!
    if (thisTx.IsCoinBase()) {
        return;
    }

    for (const CTxIn &txin : thisTx.tx->vin) {
        AddToSpends(txin.prevout, wtxid);
    }
}

bool CWallet::EncryptWallet(const SecureString &strWalletPassphrase) {
    if (IsCrypted()) {
        return false;
    }

    CKeyingMaterial _vMasterKey;

    _vMasterKey.resize(WALLET_CRYPTO_KEY_SIZE);
    GetStrongRandBytes(&_vMasterKey[0], WALLET_CRYPTO_KEY_SIZE);

    CMasterKey kMasterKey;

    kMasterKey.vchSalt.resize(WALLET_CRYPTO_SALT_SIZE);
    GetStrongRandBytes(&kMasterKey.vchSalt[0], WALLET_CRYPTO_SALT_SIZE);

    CCrypter crypter;
    int64_t nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, 25000,
                                 kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = static_cast<unsigned int>(
        2500000 / double(GetTimeMillis() - nStartTime));

    nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt,
                                 kMasterKey.nDeriveIterations,
                                 kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations =
        (kMasterKey.nDeriveIterations +
         static_cast<unsigned int>(kMasterKey.nDeriveIterations * 100 /
                                   double(GetTimeMillis() - nStartTime))) /
        2;

    if (kMasterKey.nDeriveIterations < 25000) {
        kMasterKey.nDeriveIterations = 25000;
    }

    WalletLogPrintf("Encrypting Wallet with an nDeriveIterations of %i\n",
                    kMasterKey.nDeriveIterations);

    if (!crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt,
                                      kMasterKey.nDeriveIterations,
                                      kMasterKey.nDerivationMethod)) {
        return false;
    }

    if (!crypter.Encrypt(_vMasterKey, kMasterKey.vchCryptedKey)) {
        return false;
    }

    {
        LOCK(cs_wallet);
        mapMasterKeys[++nMasterKeyMaxID] = kMasterKey;
        WalletBatch *encrypted_batch = new WalletBatch(*database);
        if (!encrypted_batch->TxnBegin()) {
            delete encrypted_batch;
            encrypted_batch = nullptr;
            return false;
        }
        encrypted_batch->WriteMasterKey(nMasterKeyMaxID, kMasterKey);

        for (const auto &spk_man_pair : m_spk_managers) {
            auto spk_man = spk_man_pair.second.get();
            if (!spk_man->Encrypt(_vMasterKey, encrypted_batch)) {
                encrypted_batch->TxnAbort();
                delete encrypted_batch;
                encrypted_batch = nullptr;
                // We now probably have half of our keys encrypted in memory,
                // and half not... die and let the user reload the unencrypted
                // wallet.
                assert(false);
            }
        }

        // Encryption was introduced in version 0.4.0
        SetMinVersion(FEATURE_WALLETCRYPT, encrypted_batch, true);

        if (!encrypted_batch->TxnCommit()) {
            delete encrypted_batch;
            encrypted_batch = nullptr;
            // We now have keys encrypted in memory, but not on disk...
            // die to avoid confusion and let the user reload the unencrypted
            // wallet.
            assert(false);
        }

        delete encrypted_batch;
        encrypted_batch = nullptr;

        Lock();
        Unlock(strWalletPassphrase);

        // If we are using descriptors, make new descriptors with a new seed
        if (IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS) &&
            !IsWalletFlagSet(WALLET_FLAG_BLANK_WALLET)) {
            SetupDescriptorScriptPubKeyMans();
        } else if (auto spk_man = GetLegacyScriptPubKeyMan()) {
            // if we are using HD, replace the HD seed with a new one
            if (spk_man->IsHDEnabled()) {
                if (!spk_man->SetupGeneration(true)) {
                    return false;
                }
            }
        }
        Lock();

        // Need to completely rewrite the wallet file; if we don't, bdb might
        // keep bits of the unencrypted private key in slack space in the
        // database file.
        database->Rewrite();

        // BDB seems to have a bad habit of writing old data into
        // slack space in .dat files; that is bad if the old data is
        // unencrypted private keys. So:
        database->ReloadDbEnv();
    }

    NotifyStatusChanged(this);
    return true;
}

DBErrors CWallet::ReorderTransactions() {
    LOCK(cs_wallet);
    WalletBatch batch(*database);

    // Old wallets didn't have any defined order for transactions. Probably a
    // bad idea to change the output of this.

    // First: get all CWalletTx into a sorted-by-time
    // multimap.
    TxItems txByTime;

    for (auto &entry : mapWallet) {
        CWalletTx *wtx = &entry.second;
        txByTime.insert(std::make_pair(wtx->nTimeReceived, wtx));
    }

    nOrderPosNext = 0;
    std::vector<int64_t> nOrderPosOffsets;
    for (TxItems::iterator it = txByTime.begin(); it != txByTime.end(); ++it) {
        CWalletTx *const pwtx = (*it).second;
        int64_t &nOrderPos = pwtx->nOrderPos;

        if (nOrderPos == -1) {
            nOrderPos = nOrderPosNext++;
            nOrderPosOffsets.push_back(nOrderPos);

            if (!batch.WriteTx(*pwtx)) {
                return DBErrors::LOAD_FAIL;
            }
        } else {
            int64_t nOrderPosOff = 0;
            for (const int64_t &nOffsetStart : nOrderPosOffsets) {
                if (nOrderPos >= nOffsetStart) {
                    ++nOrderPosOff;
                }
            }

            nOrderPos += nOrderPosOff;
            nOrderPosNext = std::max(nOrderPosNext, nOrderPos + 1);

            if (!nOrderPosOff) {
                continue;
            }

            // Since we're changing the order, write it back.
            if (!batch.WriteTx(*pwtx)) {
                return DBErrors::LOAD_FAIL;
            }
        }
    }

    batch.WriteOrderPosNext(nOrderPosNext);

    return DBErrors::LOAD_OK;
}

int64_t CWallet::IncOrderPosNext(WalletBatch *batch) {
    AssertLockHeld(cs_wallet);
    int64_t nRet = nOrderPosNext++;
    if (batch) {
        batch->WriteOrderPosNext(nOrderPosNext);
    } else {
        WalletBatch(*database).WriteOrderPosNext(nOrderPosNext);
    }

    return nRet;
}

void CWallet::MarkDirty() {
    LOCK(cs_wallet);
    for (std::pair<const TxId, CWalletTx> &item : mapWallet) {
        item.second.MarkDirty();
    }
}

void CWallet::SetSpentKeyState(WalletBatch &batch, const TxId &txid,
                               unsigned int n, bool used,
                               std::set<CTxDestination> &tx_destinations) {
    AssertLockHeld(cs_wallet);
    const CWalletTx *srctx = GetWalletTx(txid);
    if (!srctx) {
        return;
    }

    CTxDestination dst;
    if (ExtractDestination(srctx->tx->vout[n].scriptPubKey, dst)) {
        if (IsMine(dst)) {
            if (used && !GetDestData(dst, "used", nullptr)) {
                // p for "present", opposite of absent (null)
                if (AddDestData(batch, dst, "used", "p")) {
                    tx_destinations.insert(dst);
                }
            } else if (!used && GetDestData(dst, "used", nullptr)) {
                EraseDestData(batch, dst, "used");
            }
        }
    }
}

bool CWallet::IsSpentKey(const TxId &txid, unsigned int n) const {
    AssertLockHeld(cs_wallet);
    const CWalletTx *srctx = GetWalletTx(txid);
    if (srctx) {
        assert(srctx->tx->vout.size() > n);
        CTxDestination dest;
        if (!ExtractDestination(srctx->tx->vout[n].scriptPubKey, dest)) {
            return false;
        }
        if (GetDestData(dest, "used", nullptr)) {
            return true;
        }
        if (IsLegacy()) {
            LegacyScriptPubKeyMan *spk_man = GetLegacyScriptPubKeyMan();
            assert(spk_man != nullptr);
            for (const auto &keyid :
                 GetAffectedKeys(srctx->tx->vout[n].scriptPubKey, *spk_man)) {
                PKHash pkh_dest(keyid);
                if (GetDestData(pkh_dest, "used", nullptr)) {
                    return true;
                }
            }
        }
    }
    return false;
}

CWalletTx *CWallet::AddToWallet(CTransactionRef tx,
                                const CWalletTx::Confirmation &confirm,
                                const UpdateWalletTxFn &update_wtx,
                                bool fFlushOnClose) {
    LOCK(cs_wallet);

    WalletBatch batch(*database, "r+", fFlushOnClose);

    const TxId &txid = tx->GetId();

    if (IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE)) {
        // Mark used destinations
        std::set<CTxDestination> tx_destinations;

        for (const CTxIn &txin : tx->vin) {
            const COutPoint &op = txin.prevout;
            SetSpentKeyState(batch, op.GetTxId(), op.GetN(), true,
                             tx_destinations);
        }

        MarkDestinationsDirty(tx_destinations);
    }

    // Inserts only if not already there, returns tx inserted or tx found.
    auto ret =
        mapWallet.emplace(std::piecewise_construct, std::forward_as_tuple(txid),
                          std::forward_as_tuple(this, tx));
    CWalletTx &wtx = (*ret.first).second;
    bool fInsertedNew = ret.second;
    bool fUpdated = update_wtx && update_wtx(wtx, fInsertedNew);
    if (fInsertedNew) {
        wtx.m_confirm = confirm;
        wtx.nTimeReceived = chain().getAdjustedTime();
        wtx.nOrderPos = IncOrderPosNext(&batch);
        wtx.m_it_wtxOrdered =
            wtxOrdered.insert(std::make_pair(wtx.nOrderPos, &wtx));
        wtx.nTimeSmart = ComputeTimeSmart(wtx);
        AddToSpends(txid);
    }

    if (!fInsertedNew) {
        if (confirm.status != wtx.m_confirm.status) {
            wtx.m_confirm.status = confirm.status;
            wtx.m_confirm.nIndex = confirm.nIndex;
            wtx.m_confirm.hashBlock = confirm.hashBlock;
            wtx.m_confirm.block_height = confirm.block_height;
            fUpdated = true;
        } else {
            assert(wtx.m_confirm.nIndex == confirm.nIndex);
            assert(wtx.m_confirm.hashBlock == confirm.hashBlock);
            assert(wtx.m_confirm.block_height == confirm.block_height);
        }
    }

    //// debug print
    WalletLogPrintf("AddToWallet %s  %s%s\n", txid.ToString(),
                    (fInsertedNew ? "new" : ""), (fUpdated ? "update" : ""));

    // Write to disk
    if ((fInsertedNew || fUpdated) && !batch.WriteTx(wtx)) {
        return nullptr;
    }

    // Break debit/credit balance caches:
    wtx.MarkDirty();

    // Notify UI of new or updated transaction.
    NotifyTransactionChanged(this, txid, fInsertedNew ? CT_NEW : CT_UPDATED);

#if defined(HAVE_SYSTEM)
    // Notify an external script when a wallet transaction comes in or is
    // updated.
    std::string strCmd = gArgs.GetArg("-walletnotify", "");

    if (!strCmd.empty()) {
        boost::replace_all(strCmd, "%s", txid.GetHex());
#ifndef WIN32
        // Substituting the wallet name isn't currently supported on windows
        // because windows shell escaping has not been implemented yet:
        // https://github.com/bitcoin/bitcoin/pull/13339#issuecomment-537384875
        // A few ways it could be implemented in the future are described in:
        // https://github.com/bitcoin/bitcoin/pull/13339#issuecomment-461288094
        boost::replace_all(strCmd, "%w", ShellEscape(GetName()));
#endif

        std::thread t(runCommand, strCmd);
        // Thread runs free.
        t.detach();
    }
#endif

    return &wtx;
}

bool CWallet::LoadToWallet(const TxId &txid, const UpdateWalletTxFn &fill_wtx) {
    const auto &ins =
        mapWallet.emplace(std::piecewise_construct, std::forward_as_tuple(txid),
                          std::forward_as_tuple(this, nullptr));
    CWalletTx &wtx = ins.first->second;
    if (!fill_wtx(wtx, ins.second)) {
        return false;
    }
    // If wallet doesn't have a chain (e.g wallet-tool), don't bother to update
    // txn.
    if (HaveChain()) {
        std::optional<int> block_height =
            chain().getBlockHeight(wtx.m_confirm.hashBlock);
        if (block_height) {
            // Update cached block height variable since it not stored in the
            // serialized transaction.
            wtx.m_confirm.block_height = *block_height;
        } else if (wtx.isConflicted() || wtx.isConfirmed()) {
            // If tx block (or conflicting block) was reorged out of chain
            // while the wallet was shutdown, change tx status to UNCONFIRMED
            // and reset block height, hash, and index. ABANDONED tx don't have
            // associated blocks and don't need to be updated. The case where a
            // transaction was reorged out while online and then reconfirmed
            // while offline is covered by the rescan logic.
            wtx.setUnconfirmed();
            wtx.m_confirm.hashBlock = BlockHash();
            wtx.m_confirm.block_height = 0;
            wtx.m_confirm.nIndex = 0;
        }
    }
    if (/* insertion took place */ ins.second) {
        wtx.m_it_wtxOrdered =
            wtxOrdered.insert(std::make_pair(wtx.nOrderPos, &wtx));
    }
    AddToSpends(txid);
    for (const CTxIn &txin : wtx.tx->vin) {
        auto it = mapWallet.find(txin.prevout.GetTxId());
        if (it != mapWallet.end()) {
            CWalletTx &prevtx = it->second;
            if (prevtx.isConflicted()) {
                MarkConflicted(prevtx.m_confirm.hashBlock,
                               prevtx.m_confirm.block_height, wtx.GetId());
            }
        }
    }
    return true;
}

bool CWallet::AddToWalletIfInvolvingMe(const CTransactionRef &ptx,
                                       CWalletTx::Confirmation confirm,
                                       bool fUpdate) {
    const CTransaction &tx = *ptx;
    AssertLockHeld(cs_wallet);

    if (!confirm.hashBlock.IsNull()) {
        for (const CTxIn &txin : tx.vin) {
            std::pair<TxSpends::const_iterator, TxSpends::const_iterator>
                range = mapTxSpends.equal_range(txin.prevout);
            while (range.first != range.second) {
                if (range.first->second != tx.GetId()) {
                    WalletLogPrintf(
                        "Transaction %s (in block %s) conflicts with wallet "
                        "transaction %s (both spend %s:%i)\n",
                        tx.GetId().ToString(), confirm.hashBlock.ToString(),
                        range.first->second.ToString(),
                        range.first->first.GetTxId().ToString(),
                        range.first->first.GetN());
                    MarkConflicted(confirm.hashBlock, confirm.block_height,
                                   range.first->second);
                }
                range.first++;
            }
        }
    }

    bool fExisted = mapWallet.count(tx.GetId()) != 0;
    if (fExisted && !fUpdate) {
        return false;
    }
    if (fExisted || IsMine(tx) || IsFromMe(tx)) {
        /**
         * Check if any keys in the wallet keypool that were supposed to be
         * unused have appeared in a new transaction. If so, remove those keys
         * from the keypool. This can happen when restoring an old wallet backup
         * that does not contain the mostly recently created transactions from
         * newer versions of the wallet.
         */

        // loop though all outputs
        for (const CTxOut &txout : tx.vout) {
            for (const auto &spk_man_pair : m_spk_managers) {
                spk_man_pair.second->MarkUnusedAddresses(txout.scriptPubKey);
            }
        }

        // Block disconnection override an abandoned tx as unconfirmed
        // which means user may have to call abandontransaction again
        return AddToWallet(MakeTransactionRef(tx), confirm,
                           /* update_wtx= */ nullptr,
                           /* fFlushOnClose= */ false);
    }
    return false;
}

bool CWallet::TransactionCanBeAbandoned(const TxId &txid) const {
    LOCK(cs_wallet);
    const CWalletTx *wtx = GetWalletTx(txid);
    return wtx && !wtx->isAbandoned() && wtx->GetDepthInMainChain() == 0 &&
           !wtx->InMempool();
}

void CWallet::MarkInputsDirty(const CTransactionRef &tx) {
    for (const CTxIn &txin : tx->vin) {
        auto it = mapWallet.find(txin.prevout.GetTxId());
        if (it != mapWallet.end()) {
            it->second.MarkDirty();
        }
    }
}

bool CWallet::AbandonTransaction(const TxId &txid) {
    LOCK(cs_wallet);

    WalletBatch batch(*database, "r+");

    std::set<TxId> todo;
    std::set<TxId> done;

    // Can't mark abandoned if confirmed or in mempool
    auto it = mapWallet.find(txid);
    assert(it != mapWallet.end());
    CWalletTx &origtx = it->second;
    if (origtx.GetDepthInMainChain() != 0 || origtx.InMempool()) {
        return false;
    }

    todo.insert(txid);

    while (!todo.empty()) {
        const TxId now = *todo.begin();
        todo.erase(now);
        done.insert(now);
        it = mapWallet.find(now);
        assert(it != mapWallet.end());
        CWalletTx &wtx = it->second;
        int currentconfirm = wtx.GetDepthInMainChain();
        // If the orig tx was not in block, none of its spends can be.
        assert(currentconfirm <= 0);
        // If (currentconfirm < 0) {Tx and spends are already conflicted, no
        // need to abandon}
        if (currentconfirm == 0 && !wtx.isAbandoned()) {
            // If the orig tx was not in block/mempool, none of its spends can
            // be in mempool.
            assert(!wtx.InMempool());
            wtx.setAbandoned();
            wtx.MarkDirty();
            batch.WriteTx(wtx);
            NotifyTransactionChanged(this, wtx.GetId(), CT_UPDATED);
            // Iterate over all its outputs, and mark transactions in the wallet
            // that spend them abandoned too.
            TxSpends::const_iterator iter =
                mapTxSpends.lower_bound(COutPoint(now, 0));
            while (iter != mapTxSpends.end() && iter->first.GetTxId() == now) {
                if (!done.count(iter->second)) {
                    todo.insert(iter->second);
                }
                iter++;
            }

            // If a transaction changes 'conflicted' state, that changes the
            // balance available of the outputs it spends. So force those to be
            // recomputed.
            MarkInputsDirty(wtx.tx);
        }
    }

    return true;
}

void CWallet::MarkConflicted(const BlockHash &hashBlock, int conflicting_height,
                             const TxId &txid) {
    LOCK(cs_wallet);

    int conflictconfirms =
        (m_last_block_processed_height - conflicting_height + 1) * -1;

    // If number of conflict confirms cannot be determined, this means that the
    // block is still unknown or not yet part of the main chain, for example
    // when loading the wallet during a reindex. Do nothing in that case.
    if (conflictconfirms >= 0) {
        return;
    }

    // Do not flush the wallet here for performance reasons.
    WalletBatch batch(*database, "r+", false);

    std::set<TxId> todo;
    std::set<TxId> done;

    todo.insert(txid);

    while (!todo.empty()) {
        const TxId now = *todo.begin();
        todo.erase(now);
        done.insert(now);
        auto it = mapWallet.find(now);
        assert(it != mapWallet.end());
        CWalletTx &wtx = it->second;
        int currentconfirm = wtx.GetDepthInMainChain();
        if (conflictconfirms < currentconfirm) {
            // Block is 'more conflicted' than current confirm; update.
            // Mark transaction as conflicted with this block.
            wtx.m_confirm.nIndex = 0;
            wtx.m_confirm.hashBlock = hashBlock;
            wtx.m_confirm.block_height = conflicting_height;
            wtx.setConflicted();
            wtx.MarkDirty();
            batch.WriteTx(wtx);
            // Iterate over all its outputs, and mark transactions in the wallet
            // that spend them conflicted too.
            TxSpends::const_iterator iter =
                mapTxSpends.lower_bound(COutPoint(now, 0));
            while (iter != mapTxSpends.end() && iter->first.GetTxId() == now) {
                if (!done.count(iter->second)) {
                    todo.insert(iter->second);
                }
                iter++;
            }
            // If a transaction changes 'conflicted' state, that changes the
            // balance available of the outputs it spends. So force those to be
            // recomputed.
            MarkInputsDirty(wtx.tx);
        }
    }
}

void CWallet::SyncTransaction(const CTransactionRef &ptx,
                              CWalletTx::Confirmation confirm, bool update_tx) {
    if (!AddToWalletIfInvolvingMe(ptx, confirm, update_tx)) {
        // Not one of ours
        return;
    }

    // If a transaction changes 'conflicted' state, that changes the balance
    // available of the outputs it spends. So force those to be
    // recomputed, also:
    MarkInputsDirty(ptx);
}

void CWallet::transactionAddedToMempool(const CTransactionRef &ptx) {
    LOCK(cs_wallet);
    CWalletTx::Confirmation confirm(CWalletTx::Status::UNCONFIRMED,
                                    /* block_height */ 0, BlockHash(),
                                    /* nIndex */ 0);
    SyncTransaction(ptx, confirm);

    auto it = mapWallet.find(ptx->GetId());
    if (it != mapWallet.end()) {
        it->second.fInMempool = true;
    }
}

void CWallet::transactionRemovedFromMempool(const CTransactionRef &ptx) {
    LOCK(cs_wallet);
    auto it = mapWallet.find(ptx->GetId());
    if (it != mapWallet.end()) {
        it->second.fInMempool = false;
    }
}

void CWallet::blockConnected(const CBlock &block, int height) {
    const BlockHash &block_hash = block.GetHash();
    LOCK(cs_wallet);

    m_last_block_processed_height = height;
    m_last_block_processed = block_hash;
    for (size_t index = 0; index < block.vtx.size(); index++) {
        CWalletTx::Confirmation confirm(CWalletTx::Status::CONFIRMED, height,
                                        block_hash, index);
        SyncTransaction(block.vtx[index], confirm);
        transactionRemovedFromMempool(block.vtx[index]);
    }
}

void CWallet::blockDisconnected(const CBlock &block, int height) {
    LOCK(cs_wallet);

    // At block disconnection, this will change an abandoned transaction to
    // be unconfirmed, whether or not the transaction is added back to the
    // mempool. User may have to call abandontransaction again. It may be
    // addressed in the future with a stickier abandoned state or even removing
    // abandontransaction call.
    m_last_block_processed_height = height - 1;
    m_last_block_processed = block.hashPrevBlock;
    for (const CTransactionRef &ptx : block.vtx) {
        CWalletTx::Confirmation confirm(CWalletTx::Status::UNCONFIRMED,
                                        /* block_height */ 0, BlockHash(),
                                        /* nIndex */ 0);
        SyncTransaction(ptx, confirm);
    }
}

void CWallet::updatedBlockTip() {
    m_best_block_time = GetTime();
}

void CWallet::BlockUntilSyncedToCurrentChain() const {
    AssertLockNotHeld(cs_wallet);
    // Skip the queue-draining stuff if we know we're caught up with
    // chainActive.Tip(), otherwise put a callback in the validation interface
    // queue and wait for the queue to drain enough to execute it (indicating we
    // are caught up at least with the time we entered this function).
    const BlockHash last_block_hash =
        WITH_LOCK(cs_wallet, return m_last_block_processed);
    chain().waitForNotificationsIfTipChanged(last_block_hash);
}

isminetype CWallet::IsMine(const CTxIn &txin) const {
    LOCK(cs_wallet);
    std::map<TxId, CWalletTx>::const_iterator mi =
        mapWallet.find(txin.prevout.GetTxId());
    if (mi != mapWallet.end()) {
        const CWalletTx &prev = (*mi).second;
        if (txin.prevout.GetN() < prev.tx->vout.size()) {
            return IsMine(prev.tx->vout[txin.prevout.GetN()]);
        }
    }

    return ISMINE_NO;
}

// Note that this function doesn't distinguish between a 0-valued input, and a
// not-"is mine" (according to the filter) input.
Amount CWallet::GetDebit(const CTxIn &txin, const isminefilter &filter) const {
    LOCK(cs_wallet);
    std::map<TxId, CWalletTx>::const_iterator mi =
        mapWallet.find(txin.prevout.GetTxId());
    if (mi != mapWallet.end()) {
        const CWalletTx &prev = (*mi).second;
        if (txin.prevout.GetN() < prev.tx->vout.size()) {
            if (IsMine(prev.tx->vout[txin.prevout.GetN()]) & filter) {
                return prev.tx->vout[txin.prevout.GetN()].nValue;
            }
        }
    }

    return Amount::zero();
}

isminetype CWallet::IsMine(const CTxOut &txout) const {
    return IsMine(txout.scriptPubKey);
}

isminetype CWallet::IsMine(const CTxDestination &dest) const {
    return IsMine(GetScriptForDestination(dest));
}

isminetype CWallet::IsMine(const CScript &script) const {
    isminetype result = ISMINE_NO;
    for (const auto &spk_man_pair : m_spk_managers) {
        result = std::max(result, spk_man_pair.second->IsMine(script));
    }
    return result;
}

Amount CWallet::GetCredit(const CTxOut &txout,
                          const isminefilter &filter) const {
    if (!MoneyRange(txout.nValue)) {
        throw std::runtime_error(std::string(__func__) +
                                 ": value out of range");
    }

    return (IsMine(txout) & filter) ? txout.nValue : Amount::zero();
}

bool CWallet::IsChange(const CTxOut &txout) const {
    return IsChange(txout.scriptPubKey);
}

bool CWallet::IsChange(const CScript &script) const {
    // TODO: fix handling of 'change' outputs. The assumption is that any
    // payment to a script that is ours, but is not in the address book is
    // change. That assumption is likely to break when we implement
    // multisignature wallets that return change back into a
    // multi-signature-protected address; a better way of identifying which
    // outputs are 'the send' and which are 'the change' will need to be
    // implemented (maybe extend CWalletTx to remember which output, if any, was
    // change).
    if (IsMine(script)) {
        CTxDestination address;
        if (!ExtractDestination(script, address)) {
            return true;
        }

        LOCK(cs_wallet);
        if (!FindAddressBookEntry(address)) {
            return true;
        }
    }

    return false;
}

Amount CWallet::GetChange(const CTxOut &txout) const {
    if (!MoneyRange(txout.nValue)) {
        throw std::runtime_error(std::string(__func__) +
                                 ": value out of range");
    }

    return (IsChange(txout) ? txout.nValue : Amount::zero());
}

bool CWallet::IsMine(const CTransaction &tx) const {
    for (const CTxOut &txout : tx.vout) {
        if (IsMine(txout)) {
            return true;
        }
    }

    return false;
}

bool CWallet::IsFromMe(const CTransaction &tx) const {
    return GetDebit(tx, ISMINE_ALL) > Amount::zero();
}

Amount CWallet::GetDebit(const CTransaction &tx,
                         const isminefilter &filter) const {
    Amount nDebit = Amount::zero();
    for (const CTxIn &txin : tx.vin) {
        nDebit += GetDebit(txin, filter);
        if (!MoneyRange(nDebit)) {
            throw std::runtime_error(std::string(__func__) +
                                     ": value out of range");
        }
    }

    return nDebit;
}

bool CWallet::IsAllFromMe(const CTransaction &tx,
                          const isminefilter &filter) const {
    LOCK(cs_wallet);

    for (const CTxIn &txin : tx.vin) {
        auto mi = mapWallet.find(txin.prevout.GetTxId());
        if (mi == mapWallet.end()) {
            // Any unknown inputs can't be from us.
            return false;
        }

        const CWalletTx &prev = (*mi).second;

        if (txin.prevout.GetN() >= prev.tx->vout.size()) {
            // Invalid input!
            return false;
        }

        if (!(IsMine(prev.tx->vout[txin.prevout.GetN()]) & filter)) {
            return false;
        }
    }

    return true;
}

Amount CWallet::GetCredit(const CTransaction &tx,
                          const isminefilter &filter) const {
    Amount nCredit = Amount::zero();
    for (const CTxOut &txout : tx.vout) {
        nCredit += GetCredit(txout, filter);
        if (!MoneyRange(nCredit)) {
            throw std::runtime_error(std::string(__func__) +
                                     ": value out of range");
        }
    }

    return nCredit;
}

Amount CWallet::GetChange(const CTransaction &tx) const {
    Amount nChange = Amount::zero();
    for (const CTxOut &txout : tx.vout) {
        nChange += GetChange(txout);
        if (!MoneyRange(nChange)) {
            throw std::runtime_error(std::string(__func__) +
                                     ": value out of range");
        }
    }

    return nChange;
}

bool CWallet::IsHDEnabled() const {
    // All Active ScriptPubKeyMans must be HD for this to be true
    bool result = true;
    for (const auto &spk_man : GetActiveScriptPubKeyMans()) {
        result &= spk_man->IsHDEnabled();
    }
    return result;
}

bool CWallet::CanGetAddresses(bool internal) const {
    LOCK(cs_wallet);
    if (m_spk_managers.empty()) {
        return false;
    }
    for (OutputType t : OUTPUT_TYPES) {
        auto spk_man = GetScriptPubKeyMan(t, internal);
        if (spk_man && spk_man->CanGetAddresses(internal)) {
            return true;
        }
    }
    return false;
}

void CWallet::SetWalletFlag(uint64_t flags) {
    LOCK(cs_wallet);
    m_wallet_flags |= flags;
    if (!WalletBatch(*database).WriteWalletFlags(m_wallet_flags)) {
        throw std::runtime_error(std::string(__func__) +
                                 ": writing wallet flags failed");
    }
}

void CWallet::UnsetWalletFlag(uint64_t flag) {
    WalletBatch batch(*database);
    UnsetWalletFlagWithDB(batch, flag);
}

void CWallet::UnsetWalletFlagWithDB(WalletBatch &batch, uint64_t flag) {
    LOCK(cs_wallet);
    m_wallet_flags &= ~flag;
    if (!batch.WriteWalletFlags(m_wallet_flags)) {
        throw std::runtime_error(std::string(__func__) +
                                 ": writing wallet flags failed");
    }
}

void CWallet::UnsetBlankWalletFlag(WalletBatch &batch) {
    UnsetWalletFlagWithDB(batch, WALLET_FLAG_BLANK_WALLET);
}

bool CWallet::IsWalletFlagSet(uint64_t flag) const {
    return (m_wallet_flags & flag);
}

bool CWallet::LoadWalletFlags(uint64_t flags) {
    LOCK(cs_wallet);
    if (((flags & KNOWN_WALLET_FLAGS) >> 32) ^ (flags >> 32)) {
        // contains unknown non-tolerable wallet flags
        return false;
    }
    m_wallet_flags = flags;

    return true;
}

bool CWallet::AddWalletFlags(uint64_t flags) {
    LOCK(cs_wallet);
    // We should never be writing unknown onon-tolerable wallet flags
    assert(!(((flags & KNOWN_WALLET_FLAGS) >> 32) ^ (flags >> 32)));
    if (!WalletBatch(*database).WriteWalletFlags(flags)) {
        throw std::runtime_error(std::string(__func__) +
                                 ": writing wallet flags failed");
    }

    return LoadWalletFlags(flags);
}

int64_t CWalletTx::GetTxTime() const {
    int64_t n = nTimeSmart;
    return n ? n : nTimeReceived;
}

// Helper for producing a max-sized low-S low-R signature (eg 71 bytes)
// or a max-sized low-S signature (e.g. 72 bytes) if use_max_sig is true
bool CWallet::DummySignInput(CTxIn &tx_in, const CTxOut &txout,
                             bool use_max_sig) const {
    // Fill in dummy signatures for fee calculation.
    const CScript &scriptPubKey = txout.scriptPubKey;
    SignatureData sigdata;

    std::unique_ptr<SigningProvider> provider =
        GetSolvingProvider(scriptPubKey);
    if (!provider) {
        // We don't know about this scriptpbuKey;
        return false;
    }

    if (!ProduceSignature(*provider,
                          use_max_sig ? DUMMY_MAXIMUM_SIGNATURE_CREATOR
                                      : DUMMY_SIGNATURE_CREATOR,
                          scriptPubKey, sigdata)) {
        return false;
    }

    UpdateInput(tx_in, sigdata);
    return true;
}

// Helper for producing a bunch of max-sized low-S low-R signatures (eg 71
// bytes)
bool CWallet::DummySignTx(CMutableTransaction &txNew,
                          const std::vector<CTxOut> &txouts,
                          bool use_max_sig) const {
    // Fill in dummy signatures for fee calculation.
    int nIn = 0;
    for (const auto &txout : txouts) {
        if (!DummySignInput(txNew.vin[nIn], txout, use_max_sig)) {
            return false;
        }

        nIn++;
    }
    return true;
}

bool CWallet::ImportScripts(const std::set<CScript> scripts,
                            int64_t timestamp) {
    auto spk_man = GetLegacyScriptPubKeyMan();
    if (!spk_man) {
        return false;
    }
    LOCK(spk_man->cs_KeyStore);
    return spk_man->ImportScripts(scripts, timestamp);
}

bool CWallet::ImportPrivKeys(const std::map<CKeyID, CKey> &privkey_map,
                             const int64_t timestamp) {
    auto spk_man = GetLegacyScriptPubKeyMan();
    if (!spk_man) {
        return false;
    }
    LOCK(spk_man->cs_KeyStore);
    return spk_man->ImportPrivKeys(privkey_map, timestamp);
}

bool CWallet::ImportPubKeys(
    const std::vector<CKeyID> &ordered_pubkeys,
    const std::map<CKeyID, CPubKey> &pubkey_map,
    const std::map<CKeyID, std::pair<CPubKey, KeyOriginInfo>> &key_origins,
    const bool add_keypool, const bool internal, const int64_t timestamp) {
    auto spk_man = GetLegacyScriptPubKeyMan();
    if (!spk_man) {
        return false;
    }
    LOCK(spk_man->cs_KeyStore);
    return spk_man->ImportPubKeys(ordered_pubkeys, pubkey_map, key_origins,
                                  add_keypool, internal, timestamp);
}

bool CWallet::ImportScriptPubKeys(const std::string &label,
                                  const std::set<CScript> &script_pub_keys,
                                  const bool have_solving_data,
                                  const bool apply_label,
                                  const int64_t timestamp) {
    auto spk_man = GetLegacyScriptPubKeyMan();
    if (!spk_man) {
        return false;
    }
    LOCK(spk_man->cs_KeyStore);
    if (!spk_man->ImportScriptPubKeys(script_pub_keys, have_solving_data,
                                      timestamp)) {
        return false;
    }
    if (apply_label) {
        WalletBatch batch(*database);
        for (const CScript &script : script_pub_keys) {
            CTxDestination dest;
            ExtractDestination(script, dest);
            if (IsValidDestination(dest)) {
                SetAddressBookWithDB(batch, dest, label, "receive");
            }
        }
    }
    return true;
}

int64_t CalculateMaximumSignedTxSize(const CTransaction &tx,
                                     const CWallet *wallet, bool use_max_sig) {
    std::vector<CTxOut> txouts;
    for (auto &input : tx.vin) {
        const auto mi = wallet->mapWallet.find(input.prevout.GetTxId());
        // Can not estimate size without knowing the input details
        if (mi == wallet->mapWallet.end()) {
            return -1;
        }
        assert(input.prevout.GetN() < mi->second.tx->vout.size());
        txouts.emplace_back(mi->second.tx->vout[input.prevout.GetN()]);
    }
    return CalculateMaximumSignedTxSize(tx, wallet, txouts, use_max_sig);
}

// txouts needs to be in the order of tx.vin
int64_t CalculateMaximumSignedTxSize(const CTransaction &tx,
                                     const CWallet *wallet,
                                     const std::vector<CTxOut> &txouts,
                                     bool use_max_sig) {
    CMutableTransaction txNew(tx);
    if (!wallet->DummySignTx(txNew, txouts, use_max_sig)) {
        return -1;
    }
    return GetSerializeSize(txNew, PROTOCOL_VERSION);
}

int CalculateMaximumSignedInputSize(const CTxOut &txout, const CWallet *wallet,
                                    bool use_max_sig) {
    CMutableTransaction txn;
    txn.vin.push_back(CTxIn(COutPoint()));
    if (!wallet->DummySignInput(txn.vin[0], txout, use_max_sig)) {
        return -1;
    }
    return GetSerializeSize(txn.vin[0], PROTOCOL_VERSION);
}

void CWalletTx::GetAmounts(std::list<COutputEntry> &listReceived,
                           std::list<COutputEntry> &listSent, Amount &nFee,
                           const isminefilter &filter) const {
    nFee = Amount::zero();
    listReceived.clear();
    listSent.clear();

    // Compute fee:
    Amount nDebit = GetDebit(filter);
    // debit>0 means we signed/sent this transaction.
    if (nDebit > Amount::zero()) {
        Amount nValueOut = tx->GetValueOut();
        nFee = (nDebit - nValueOut);
    }

    // Sent/received.
    for (unsigned int i = 0; i < tx->vout.size(); ++i) {
        const CTxOut &txout = tx->vout[i];
        isminetype fIsMine = pwallet->IsMine(txout);
        // Only need to handle txouts if AT LEAST one of these is true:
        //   1) they debit from us (sent)
        //   2) the output is to us (received)
        if (nDebit > Amount::zero()) {
            // Don't report 'change' txouts
            if (pwallet->IsChange(txout)) {
                continue;
            }
        } else if (!(fIsMine & filter)) {
            continue;
        }

        // In either case, we need to get the destination address.
        CTxDestination address;

        if (!ExtractDestination(txout.scriptPubKey, address) &&
            !txout.scriptPubKey.IsUnspendable()) {
            pwallet->WalletLogPrintf("CWalletTx::GetAmounts: Unknown "
                                     "transaction type found, txid %s\n",
                                     this->GetId().ToString());
            address = CNoDestination();
        }

        COutputEntry output = {address, txout.nValue, (int)i};

        // If we are debited by the transaction, add the output as a "sent"
        // entry.
        if (nDebit > Amount::zero()) {
            listSent.push_back(output);
        }

        // If we are receiving the output, add it as a "received" entry.
        if (fIsMine & filter) {
            listReceived.push_back(output);
        }
    }
}

/**
 * Scan active chain for relevant transactions after importing keys. This should
 * be called whenever new keys are added to the wallet, with the oldest key
 * creation time.
 *
 * @return Earliest timestamp that could be successfully scanned from. Timestamp
 * returned will be higher than startTime if relevant blocks could not be read.
 */
int64_t CWallet::RescanFromTime(int64_t startTime,
                                const WalletRescanReserver &reserver,
                                bool update) {
    // Find starting block. May be null if nCreateTime is greater than the
    // highest blockchain timestamp, in which case there is nothing that needs
    // to be scanned.
    int start_height = 0;
    BlockHash start_block;
    bool start = chain().findFirstBlockWithTimeAndHeight(
        startTime - TIMESTAMP_WINDOW, 0,
        FoundBlock().hash(start_block).height(start_height));
    WalletLogPrintf("%s: Rescanning last %i blocks\n", __func__,
                    start ? WITH_LOCK(cs_wallet, return GetLastBlockHeight()) -
                                start_height + 1
                          : 0);

    if (start) {
        // TODO: this should take into account failure by ScanResult::USER_ABORT
        ScanResult result = ScanForWalletTransactions(
            start_block, start_height, {} /* max_height */, reserver, update);
        if (result.status == ScanResult::FAILURE) {
            int64_t time_max;
            CHECK_NONFATAL(chain().findBlock(result.last_failed_block,
                                             FoundBlock().maxTime(time_max)));
            return time_max + TIMESTAMP_WINDOW + 1;
        }
    }
    return startTime;
}

/**
 * Scan the block chain (starting in start_block) for transactions from or to
 * us. If fUpdate is true, found transactions that already exist in the wallet
 * will be updated.
 *
 * @param[in] start_block Scan starting block. If block is not on the active
 *                        chain, the scan will return SUCCESS immediately.
 * @param[in] start_height Height of start_block
 * @param[in] max_height  Optional max scanning height. If unset there is
 *                        no maximum and scanning can continue to the tip
 *
 * @return ScanResult returning scan information and indicating success or
 *         failure. Return status will be set to SUCCESS if scan was
 *         successful. FAILURE if a complete rescan was not possible (due to
 *         pruning or corruption). USER_ABORT if the rescan was aborted before
 *         it could complete.
 *
 * @pre Caller needs to make sure start_block (and the optional stop_block) are
 * on the main chain after to the addition of any new keys you want to detect
 * transactions for.
 */
CWallet::ScanResult CWallet::ScanForWalletTransactions(
    const BlockHash &start_block, int start_height,
    std::optional<int> max_height, const WalletRescanReserver &reserver,
    bool fUpdate) {
    int64_t nNow = GetTime();
    int64_t start_time = GetTimeMillis();

    assert(reserver.isReserved());

    BlockHash block_hash = start_block;
    ScanResult result;

    WalletLogPrintf("Rescan started from block %s...\n",
                    start_block.ToString());

    fAbortRescan = false;
    // Show rescan progress in GUI as dialog or on splashscreen, if -rescan on
    // startup.
    ShowProgress(
        strprintf("%s " + _("Rescanning...").translated, GetDisplayName()), 0);
    BlockHash tip_hash = WITH_LOCK(cs_wallet, return GetLastBlockHash());
    BlockHash end_hash = tip_hash;
    if (max_height) {
        chain().findAncestorByHeight(tip_hash, *max_height,
                                     FoundBlock().hash(end_hash));
    }
    double progress_begin = chain().guessVerificationProgress(block_hash);
    double progress_end = chain().guessVerificationProgress(end_hash);
    double progress_current = progress_begin;
    int block_height = start_height;
    while (!fAbortRescan && !chain().shutdownRequested()) {
        m_scanning_progress = (progress_current - progress_begin) /
                              (progress_end - progress_begin);
        if (block_height % 100 == 0 && progress_end - progress_begin > 0.0) {
            ShowProgress(
                strprintf("%s " + _("Rescanning...").translated,
                          GetDisplayName()),
                std::max(1, std::min(99, (int)(m_scanning_progress * 100))));
        }
        if (GetTime() >= nNow + 60) {
            nNow = GetTime();
            WalletLogPrintf("Still rescanning. At block %d. Progress=%f\n",
                            block_height, progress_current);
        }

        CBlock block;
        bool next_block;
        BlockHash next_block_hash;
        bool reorg = false;
        if (chain().findBlock(block_hash, FoundBlock().data(block)) &&
            !block.IsNull()) {
            LOCK(cs_wallet);
            next_block = chain().findNextBlock(
                block_hash, block_height, FoundBlock().hash(next_block_hash),
                &reorg);
            if (reorg) {
                // Abort scan if current block is no longer active, to prevent
                // marking transactions as coming from the wrong block.
                // TODO: This should return success instead of failure, see
                // https://github.com/bitcoin/bitcoin/pull/14711#issuecomment-458342518
                result.last_failed_block = block_hash;
                result.status = ScanResult::FAILURE;
                break;
            }
            for (size_t posInBlock = 0; posInBlock < block.vtx.size();
                 ++posInBlock) {
                CWalletTx::Confirmation confirm(CWalletTx::Status::CONFIRMED,
                                                block_height, block_hash,
                                                posInBlock);
                SyncTransaction(block.vtx[posInBlock], confirm, fUpdate);
            }
            // scan succeeded, record block as most recent successfully
            // scanned
            result.last_scanned_block = block_hash;
            result.last_scanned_height = block_height;
        } else {
            // could not scan block, keep scanning but record this block as
            // the most recent failure
            result.last_failed_block = block_hash;
            result.status = ScanResult::FAILURE;
            next_block = chain().findNextBlock(
                block_hash, block_height, FoundBlock().hash(next_block_hash),
                &reorg);
        }
        if (max_height && block_height >= *max_height) {
            break;
        }
        {
            if (!next_block || reorg) {
                // break successfully when rescan has reached the tip, or
                // previous block is no longer on the chain due to a reorg
                break;
            }

            // increment block and verification progress
            block_hash = next_block_hash;
            ++block_height;
            progress_current = chain().guessVerificationProgress(block_hash);

            // handle updated tip hash
            const BlockHash prev_tip_hash = tip_hash;
            tip_hash = WITH_LOCK(cs_wallet, return GetLastBlockHash());
            if (!max_height && prev_tip_hash != tip_hash) {
                // in case the tip has changed, update progress max
                progress_end = chain().guessVerificationProgress(tip_hash);
            }
        }
    }

    // Hide progress dialog in GUI.
    ShowProgress(
        strprintf("%s " + _("Rescanning...").translated, GetDisplayName()),
        100);
    if (block_height && fAbortRescan) {
        WalletLogPrintf("Rescan aborted at block %d. Progress=%f\n",
                        block_height, progress_current);
        result.status = ScanResult::USER_ABORT;
    } else if (block_height && chain().shutdownRequested()) {
        WalletLogPrintf(
            "Rescan interrupted by shutdown request at block %d. Progress=%f\n",
            block_height, progress_current);
        result.status = ScanResult::USER_ABORT;
    } else {
        WalletLogPrintf("Rescan completed in %15dms\n",
                        GetTimeMillis() - start_time);
    }
    return result;
}

void CWallet::ReacceptWalletTransactions() {
    // If transactions aren't being broadcasted, don't let them into local
    // mempool either.
    if (!fBroadcastTransactions) {
        return;
    }

    std::map<int64_t, CWalletTx *> mapSorted;

    // Sort pending wallet transactions based on their initial wallet insertion
    // order.
    for (std::pair<const TxId, CWalletTx> &item : mapWallet) {
        const TxId &wtxid = item.first;
        CWalletTx &wtx = item.second;
        assert(wtx.GetId() == wtxid);

        int nDepth = wtx.GetDepthInMainChain();

        if (!wtx.IsCoinBase() && (nDepth == 0 && !wtx.isAbandoned())) {
            mapSorted.insert(std::make_pair(wtx.nOrderPos, &wtx));
        }
    }

    // Try to add wallet transactions to memory pool.
    for (const std::pair<const int64_t, CWalletTx *> &item : mapSorted) {
        CWalletTx &wtx = *(item.second);
        std::string unused_err_string;
        wtx.SubmitMemoryPoolAndRelay(unused_err_string, false);
    }
}

bool CWalletTx::SubmitMemoryPoolAndRelay(std::string &err_string, bool relay) {
    // Can't relay if wallet is not broadcasting
    if (!pwallet->GetBroadcastTransactions()) {
        return false;
    }
    // Don't relay abandoned transactions
    if (isAbandoned()) {
        return false;
    }
    // Don't try to submit coinbase transactions. These would fail anyway but
    // would cause log spam.
    if (IsCoinBase()) {
        return false;
    }
    // Don't try to submit conflicted or confirmed transactions.
    if (GetDepthInMainChain() != 0) {
        return false;
    }

    // Submit transaction to mempool for relay
    pwallet->WalletLogPrintf("Submitting wtx %s to mempool for relay\n",
                             GetId().ToString());
    // We must set fInMempool here - while it will be re-set to true by the
    // entered-mempool callback, if we did not there would be a race where a
    // user could call sendmoney in a loop and hit spurious out of funds errors
    // because we think that this newly generated transaction's change is
    // unavailable as we're not yet aware that it is in the mempool.
    //
    // Irrespective of the failure reason, un-marking fInMempool
    // out-of-order is incorrect - it should be unmarked when
    // TransactionRemovedFromMempool fires.
    bool ret = pwallet->chain().broadcastTransaction(
        GetConfig(), tx, pwallet->m_default_max_tx_fee, relay, err_string);
    fInMempool |= ret;
    return ret;
}

std::set<TxId> CWalletTx::GetConflicts() const {
    std::set<TxId> result;
    if (pwallet != nullptr) {
        const TxId &txid = GetId();
        result = pwallet->GetConflicts(txid);
        result.erase(txid);
    }

    return result;
}

Amount CWalletTx::GetCachableAmount(AmountType type, const isminefilter &filter,
                                    bool recalculate) const {
    auto &amount = m_amounts[type];
    if (recalculate || !amount.m_cached[filter]) {
        amount.Set(filter, type == DEBIT ? pwallet->GetDebit(*tx, filter)
                                         : pwallet->GetCredit(*tx, filter));
        m_is_cache_empty = false;
    }
    return amount.m_value[filter];
}

Amount CWalletTx::GetDebit(const isminefilter &filter) const {
    if (tx->vin.empty()) {
        return Amount::zero();
    }

    Amount debit = Amount::zero();
    if (filter & ISMINE_SPENDABLE) {
        debit += GetCachableAmount(DEBIT, ISMINE_SPENDABLE);
    }
    if (filter & ISMINE_WATCH_ONLY) {
        debit += GetCachableAmount(DEBIT, ISMINE_WATCH_ONLY);
    }

    return debit;
}

Amount CWalletTx::GetCredit(const isminefilter &filter) const {
    // Must wait until coinbase is safely deep enough in the chain before
    // valuing it.
    if (IsImmatureCoinBase()) {
        return Amount::zero();
    }

    Amount credit = Amount::zero();
    if (filter & ISMINE_SPENDABLE) {
        // GetBalance can assume transactions in mapWallet won't change.
        credit += GetCachableAmount(CREDIT, ISMINE_SPENDABLE);
    }

    if (filter & ISMINE_WATCH_ONLY) {
        credit += GetCachableAmount(CREDIT, ISMINE_WATCH_ONLY);
    }

    return credit;
}

Amount CWalletTx::GetImmatureCredit(bool fUseCache) const {
    if (IsImmatureCoinBase() && IsInMainChain()) {
        return GetCachableAmount(IMMATURE_CREDIT, ISMINE_SPENDABLE, !fUseCache);
    }

    return Amount::zero();
}

Amount CWalletTx::GetAvailableCredit(bool fUseCache,
                                     const isminefilter &filter) const {
    if (pwallet == nullptr) {
        return Amount::zero();
    }

    // Avoid caching ismine for NO or ALL cases (could remove this check and
    // simplify in the future).
    bool allow_cache =
        (filter & ISMINE_ALL) && (filter & ISMINE_ALL) != ISMINE_ALL;

    // Must wait until coinbase is safely deep enough in the chain before
    // valuing it.
    if (IsImmatureCoinBase()) {
        return Amount::zero();
    }

    if (fUseCache && allow_cache &&
        m_amounts[AVAILABLE_CREDIT].m_cached[filter]) {
        return m_amounts[AVAILABLE_CREDIT].m_value[filter];
    }

    bool allow_used_addresses =
        (filter & ISMINE_USED) ||
        !pwallet->IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE);
    Amount nCredit = Amount::zero();
    const TxId &txid = GetId();
    for (uint32_t i = 0; i < tx->vout.size(); i++) {
        if (!pwallet->IsSpent(COutPoint(txid, i)) &&
            (allow_used_addresses || !pwallet->IsSpentKey(txid, i))) {
            const CTxOut &txout = tx->vout[i];
            nCredit += pwallet->GetCredit(txout, filter);
            if (!MoneyRange(nCredit)) {
                throw std::runtime_error(std::string(__func__) +
                                         " : value out of range");
            }
        }
    }

    if (allow_cache) {
        m_amounts[AVAILABLE_CREDIT].Set(filter, nCredit);
        m_is_cache_empty = false;
    }

    return nCredit;
}

Amount CWalletTx::GetImmatureWatchOnlyCredit(const bool fUseCache) const {
    if (IsImmatureCoinBase() && IsInMainChain()) {
        return GetCachableAmount(IMMATURE_CREDIT, ISMINE_WATCH_ONLY,
                                 !fUseCache);
    }

    return Amount::zero();
}

Amount CWalletTx::GetChange() const {
    if (fChangeCached) {
        return nChangeCached;
    }

    nChangeCached = pwallet->GetChange(*tx);
    fChangeCached = true;
    return nChangeCached;
}

bool CWalletTx::InMempool() const {
    return fInMempool;
}

bool CWalletTx::IsTrusted() const {
    std::set<TxId> s;
    return IsTrusted(s);
}

bool CWalletTx::IsTrusted(std::set<TxId> &trusted_parents) const {
    // Quick answer in most cases
    TxValidationState state;
    if (!pwallet->chain().contextualCheckTransactionForCurrentBlock(*tx,
                                                                    state)) {
        return false;
    }

    int nDepth = GetDepthInMainChain();
    if (nDepth >= 1) {
        return true;
    }

    if (nDepth < 0) {
        return false;
    }

    // using wtx's cached debit
    if (!pwallet->m_spend_zero_conf_change || !IsFromMe(ISMINE_ALL)) {
        return false;
    }

    // Don't trust unconfirmed transactions from us unless they are in the
    // mempool.
    if (!InMempool()) {
        return false;
    }

    // Trusted if all inputs are from us and are in the mempool:
    for (const CTxIn &txin : tx->vin) {
        // Transactions not sent by us: not trusted
        const CWalletTx *parent = pwallet->GetWalletTx(txin.prevout.GetTxId());
        if (parent == nullptr) {
            return false;
        }

        const CTxOut &parentOut = parent->tx->vout[txin.prevout.GetN()];
        // Check that this specific input being spent is trusted
        if (pwallet->IsMine(parentOut) != ISMINE_SPENDABLE) {
            return false;
        }
        // If we've already trusted this parent, continue
        if (trusted_parents.count(parent->GetId())) {
            continue;
        }
        // Recurse to check that the parent is also trusted
        if (!parent->IsTrusted(trusted_parents)) {
            return false;
        }
        trusted_parents.insert(parent->GetId());
    }

    return true;
}

bool CWalletTx::IsEquivalentTo(const CWalletTx &_tx) const {
    CMutableTransaction tx1{*this->tx};
    CMutableTransaction tx2{*_tx.tx};
    for (auto &txin : tx1.vin) {
        txin.scriptSig = CScript();
    }

    for (auto &txin : tx2.vin) {
        txin.scriptSig = CScript();
    }

    return CTransaction(tx1) == CTransaction(tx2);
}

// Rebroadcast transactions from the wallet. We do this on a random timer
// to slightly obfuscate which transactions come from our wallet.
//
// Ideally, we'd only resend transactions that we think should have been
// mined in the most recent block. Any transaction that wasn't in the top
// blockweight of transactions in the mempool shouldn't have been mined,
// and so is probably just sitting in the mempool waiting to be confirmed.
// Rebroadcasting does nothing to speed up confirmation and only damages
// privacy.
void CWallet::ResendWalletTransactions() {
    // During reindex, importing and IBD, old wallet transactions become
    // unconfirmed. Don't resend them as that would spam other nodes.
    if (!chain().isReadyToBroadcast()) {
        return;
    }

    // Do this infrequently and randomly to avoid giving away that these are our
    // transactions.
    if (GetTime() < nNextResend || !fBroadcastTransactions) {
        return;
    }

    bool fFirst = (nNextResend == 0);
    // resend 12-36 hours from now, ~1 day on average.
    nNextResend = GetTime() + (12 * 60 * 60) + GetRand(24 * 60 * 60);
    if (fFirst) {
        return;
    }

    int submitted_tx_count = 0;

    { // cs_wallet scope
        LOCK(cs_wallet);

        // Relay transactions
        for (std::pair<const TxId, CWalletTx> &item : mapWallet) {
            CWalletTx &wtx = item.second;
            // Attempt to rebroadcast all txes more than 5 minutes older than
            // the last block. SubmitMemoryPoolAndRelay() will not rebroadcast
            // any confirmed or conflicting txs.
            if (wtx.nTimeReceived > m_best_block_time - 5 * 60) {
                continue;
            }
            std::string unused_err_string;
            if (wtx.SubmitMemoryPoolAndRelay(unused_err_string, true)) {
                ++submitted_tx_count;
            }
        }
    } // cs_wallet

    if (submitted_tx_count > 0) {
        WalletLogPrintf("%s: resubmit %u unconfirmed transactions\n", __func__,
                        submitted_tx_count);
    }
}

/** @} */ // end of mapWallet

void MaybeResendWalletTxs() {
    for (const std::shared_ptr<CWallet> &pwallet : GetWallets()) {
        pwallet->ResendWalletTransactions();
    }
}

/**
 * @defgroup Actions
 *
 * @{
 */
CWallet::Balance CWallet::GetBalance(const int min_depth,
                                     bool avoid_reuse) const {
    Balance ret;
    isminefilter reuse_filter = avoid_reuse ? ISMINE_NO : ISMINE_USED;
    LOCK(cs_wallet);
    std::set<TxId> trusted_parents;
    for (const auto &entry : mapWallet) {
        const CWalletTx &wtx = entry.second;
        const bool is_trusted{wtx.IsTrusted(trusted_parents)};
        const int tx_depth{wtx.GetDepthInMainChain()};
        const Amount tx_credit_mine{wtx.GetAvailableCredit(
            /* fUseCache */ true, ISMINE_SPENDABLE | reuse_filter)};
        const Amount tx_credit_watchonly{wtx.GetAvailableCredit(
            /* fUseCache */ true, ISMINE_WATCH_ONLY | reuse_filter)};
        if (is_trusted && tx_depth >= min_depth) {
            ret.m_mine_trusted += tx_credit_mine;
            ret.m_watchonly_trusted += tx_credit_watchonly;
        }
        if (!is_trusted && tx_depth == 0 && wtx.InMempool()) {
            ret.m_mine_untrusted_pending += tx_credit_mine;
            ret.m_watchonly_untrusted_pending += tx_credit_watchonly;
        }
        ret.m_mine_immature += wtx.GetImmatureCredit();
        ret.m_watchonly_immature += wtx.GetImmatureWatchOnlyCredit();
    }
    return ret;
}

Amount CWallet::GetAvailableBalance(const CCoinControl *coinControl) const {
    LOCK(cs_wallet);

    Amount balance = Amount::zero();
    std::vector<COutput> vCoins;
    AvailableCoins(vCoins, true, coinControl);
    for (const COutput &out : vCoins) {
        if (out.fSpendable) {
            balance += out.tx->tx->vout[out.i].nValue;
        }
    }
    return balance;
}

void CWallet::AvailableCoins(std::vector<COutput> &vCoins, bool fOnlySafe,
                             const CCoinControl *coinControl,
                             const Amount nMinimumAmount,
                             const Amount nMaximumAmount,
                             const Amount nMinimumSumAmount,
                             const uint64_t nMaximumCount) const {
    AssertLockHeld(cs_wallet);

    vCoins.clear();
    Amount nTotal = Amount::zero();
    // Either the WALLET_FLAG_AVOID_REUSE flag is not set (in which case we
    // always allow), or we default to avoiding, and only in the case where a
    // coin control object is provided, and has the avoid address reuse flag set
    // to false, do we allow already used addresses
    bool allow_used_addresses =
        !IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE) ||
        (coinControl && !coinControl->m_avoid_address_reuse);
    const int min_depth = {coinControl ? coinControl->m_min_depth
                                       : DEFAULT_MIN_DEPTH};
    const int max_depth = {coinControl ? coinControl->m_max_depth
                                       : DEFAULT_MAX_DEPTH};

    std::set<TxId> trusted_parents;
    for (const auto &entry : mapWallet) {
        const TxId &wtxid = entry.first;
        const CWalletTx &wtx = entry.second;

        TxValidationState state;
        if (!chain().contextualCheckTransactionForCurrentBlock(*wtx.tx,
                                                               state)) {
            continue;
        }

        if (wtx.IsImmatureCoinBase()) {
            continue;
        }

        int nDepth = wtx.GetDepthInMainChain();
        if (nDepth < 0) {
            continue;
        }

        // We should not consider coins which aren't at least in our mempool.
        // It's possible for these to be conflicted via ancestors which we may
        // never be able to detect.
        if (nDepth == 0 && !wtx.InMempool()) {
            continue;
        }

        bool safeTx = wtx.IsTrusted(trusted_parents);

        // Bitcoin-ABC: Removed check that prevents consideration of coins from
        // transactions that are replacing other transactions. This check based
        // on wtx.mapValue.count("replaces_txid") which was not being set
        // anywhere.

        // Similarly, we should not consider coins from transactions that have
        // been replaced. In the example above, we would want to prevent
        // creation of a transaction A' spending an output of A, because if
        // transaction B were initially confirmed, conflicting with A and A', we
        // wouldn't want to the user to create a transaction D intending to
        // replace A', but potentially resulting in a scenario where A, A', and
        // D could all be accepted (instead of just B and D, or just A and A'
        // like the user would want).

        // Bitcoin-ABC: retained this check as 'replaced_by_txid' is still set
        // in the wallet code.
        if (nDepth == 0 && wtx.mapValue.count("replaced_by_txid")) {
            safeTx = false;
        }

        if (fOnlySafe && !safeTx) {
            continue;
        }

        if (nDepth < min_depth || nDepth > max_depth) {
            continue;
        }

        for (uint32_t i = 0; i < wtx.tx->vout.size(); i++) {
            // Only consider selected coins if add_inputs is false
            if (coinControl && !coinControl->m_add_inputs &&
                !coinControl->IsSelected(COutPoint(entry.first, i))) {
                continue;
            }

            if (wtx.tx->vout[i].nValue < nMinimumAmount ||
                wtx.tx->vout[i].nValue > nMaximumAmount) {
                continue;
            }

            const COutPoint outpoint(wtxid, i);

            if (coinControl && coinControl->HasSelected() &&
                !coinControl->fAllowOtherInputs &&
                !coinControl->IsSelected(outpoint)) {
                continue;
            }

            if (IsLockedCoin(outpoint)) {
                continue;
            }

            if (IsSpent(outpoint)) {
                continue;
            }

            isminetype mine = IsMine(wtx.tx->vout[i]);

            if (mine == ISMINE_NO) {
                continue;
            }

            if (!allow_used_addresses && IsSpentKey(wtxid, i)) {
                continue;
            }

            std::unique_ptr<SigningProvider> provider =
                GetSolvingProvider(wtx.tx->vout[i].scriptPubKey);

            bool solvable =
                provider ? IsSolvable(*provider, wtx.tx->vout[i].scriptPubKey)
                         : false;
            bool spendable =
                ((mine & ISMINE_SPENDABLE) != ISMINE_NO) ||
                (((mine & ISMINE_WATCH_ONLY) != ISMINE_NO) &&
                 (coinControl && coinControl->fAllowWatchOnly && solvable));

            vCoins.push_back(
                COutput(&wtx, i, nDepth, spendable, solvable, safeTx,
                        (coinControl && coinControl->fAllowWatchOnly)));

            // Checks the sum amount of all UTXO's.
            if (nMinimumSumAmount != MAX_MONEY) {
                nTotal += wtx.tx->vout[i].nValue;

                if (nTotal >= nMinimumSumAmount) {
                    return;
                }
            }

            // Checks the maximum number of UTXO's.
            if (nMaximumCount > 0 && vCoins.size() >= nMaximumCount) {
                return;
            }
        }
    }
}

std::map<CTxDestination, std::vector<COutput>> CWallet::ListCoins() const {
    AssertLockHeld(cs_wallet);

    std::map<CTxDestination, std::vector<COutput>> result;
    std::vector<COutput> availableCoins;

    AvailableCoins(availableCoins);

    for (const auto &coin : availableCoins) {
        CTxDestination address;
        if ((coin.fSpendable ||
             (IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS) &&
              coin.fSolvable)) &&
            ExtractDestination(
                FindNonChangeParentOutput(*coin.tx->tx, coin.i).scriptPubKey,
                address)) {
            result[address].emplace_back(std::move(coin));
        }
    }

    std::vector<COutPoint> lockedCoins;
    ListLockedCoins(lockedCoins);
    // Include watch-only for LegacyScriptPubKeyMan wallets without private keys
    const bool include_watch_only =
        GetLegacyScriptPubKeyMan() &&
        IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
    const isminetype is_mine_filter =
        include_watch_only ? ISMINE_WATCH_ONLY : ISMINE_SPENDABLE;
    for (const auto &output : lockedCoins) {
        auto it = mapWallet.find(output.GetTxId());
        if (it != mapWallet.end()) {
            int depth = it->second.GetDepthInMainChain();
            if (depth >= 0 && output.GetN() < it->second.tx->vout.size() &&
                IsMine(it->second.tx->vout[output.GetN()]) == is_mine_filter) {
                CTxDestination address;
                if (ExtractDestination(
                        FindNonChangeParentOutput(*it->second.tx, output.GetN())
                            .scriptPubKey,
                        address)) {
                    result[address].emplace_back(
                        &it->second, output.GetN(), depth, true /* spendable */,
                        true /* solvable */, false /* safe */);
                }
            }
        }
    }

    return result;
}

const CTxOut &CWallet::FindNonChangeParentOutput(const CTransaction &tx,
                                                 int output) const {
    const CTransaction *ptx = &tx;
    int n = output;
    while (IsChange(ptx->vout[n]) && ptx->vin.size() > 0) {
        const COutPoint &prevout = ptx->vin[0].prevout;
        auto it = mapWallet.find(prevout.GetTxId());
        if (it == mapWallet.end() ||
            it->second.tx->vout.size() <= prevout.GetN() ||
            !IsMine(it->second.tx->vout[prevout.GetN()])) {
            break;
        }
        ptx = it->second.tx.get();
        n = prevout.GetN();
    }
    return ptx->vout[n];
}

bool CWallet::SelectCoinsMinConf(
    const Amount nTargetValue, const CoinEligibilityFilter &eligibility_filter,
    std::vector<OutputGroup> groups, std::set<CInputCoin> &setCoinsRet,
    Amount &nValueRet, const CoinSelectionParams &coin_selection_params,
    bool &bnb_used) const {
    setCoinsRet.clear();
    nValueRet = Amount::zero();

    std::vector<OutputGroup> utxo_pool;
    if (coin_selection_params.use_bnb) {
        // Get long term estimate
        CCoinControl temp;
        temp.m_confirm_target = 1008;
        CFeeRate long_term_feerate = GetMinimumFeeRate(*this, temp);

        // Calculate cost of change
        Amount cost_of_change = chain().relayDustFee().GetFee(
                                    coin_selection_params.change_spend_size) +
                                coin_selection_params.effective_fee.GetFee(
                                    coin_selection_params.change_output_size);

        // Filter by the min conf specs and add to utxo_pool and calculate
        // effective value
        for (OutputGroup &group : groups) {
            if (!group.EligibleForSpending(eligibility_filter)) {
                continue;
            }

            group.fee = Amount::zero();
            group.long_term_fee = Amount::zero();
            group.effective_value = Amount::zero();
            for (auto it = group.m_outputs.begin();
                 it != group.m_outputs.end();) {
                const CInputCoin &coin = *it;
                Amount effective_value =
                    coin.txout.nValue -
                    (coin.m_input_bytes < 0
                         ? Amount::zero()
                         : coin_selection_params.effective_fee.GetFee(
                               coin.m_input_bytes));
                // Only include outputs that are positive effective value (i.e.
                // not dust)
                if (effective_value > Amount::zero()) {
                    group.fee +=
                        coin.m_input_bytes < 0
                            ? Amount::zero()
                            : coin_selection_params.effective_fee.GetFee(
                                  coin.m_input_bytes);
                    group.long_term_fee +=
                        coin.m_input_bytes < 0
                            ? Amount::zero()
                            : long_term_feerate.GetFee(coin.m_input_bytes);
                    if (coin_selection_params.m_subtract_fee_outputs) {
                        group.effective_value += coin.txout.nValue;
                    } else {
                        group.effective_value += effective_value;
                    }
                    ++it;
                } else {
                    it = group.Discard(coin);
                }
            }
            if (group.effective_value > Amount::zero()) {
                utxo_pool.push_back(group);
            }
        }
        // Calculate the fees for things that aren't inputs
        Amount not_input_fees = coin_selection_params.effective_fee.GetFee(
            coin_selection_params.tx_noinputs_size);
        bnb_used = true;
        return SelectCoinsBnB(utxo_pool, nTargetValue, cost_of_change,
                              setCoinsRet, nValueRet, not_input_fees);
    } else {
        // Filter by the min conf specs and add to utxo_pool
        for (const OutputGroup &group : groups) {
            if (!group.EligibleForSpending(eligibility_filter)) {
                continue;
            }
            utxo_pool.push_back(group);
        }
        bnb_used = false;
        return KnapsackSolver(nTargetValue, utxo_pool, setCoinsRet, nValueRet);
    }
}

bool CWallet::SelectCoins(const std::vector<COutput> &vAvailableCoins,
                          const Amount nTargetValue,
                          std::set<CInputCoin> &setCoinsRet, Amount &nValueRet,
                          const CCoinControl &coin_control,
                          CoinSelectionParams &coin_selection_params,
                          bool &bnb_used) const {
    std::vector<COutput> vCoins(vAvailableCoins);
    Amount value_to_select = nTargetValue;

    // Default to bnb was not used. If we use it, we set it later
    bnb_used = false;

    // coin control -> return all selected outputs (we want all selected to go
    // into the transaction for sure)
    if (coin_control.HasSelected() && !coin_control.fAllowOtherInputs) {
        for (const COutput &out : vCoins) {
            if (!out.fSpendable) {
                continue;
            }

            nValueRet += out.tx->tx->vout[out.i].nValue;
            setCoinsRet.insert(out.GetInputCoin());
        }

        return (nValueRet >= nTargetValue);
    }

    // Calculate value from preset inputs and store them.
    std::set<CInputCoin> setPresetCoins;
    Amount nValueFromPresetInputs = Amount::zero();

    std::vector<COutPoint> vPresetInputs;
    coin_control.ListSelected(vPresetInputs);

    for (const COutPoint &outpoint : vPresetInputs) {
        std::map<TxId, CWalletTx>::const_iterator it =
            mapWallet.find(outpoint.GetTxId());
        if (it != mapWallet.end()) {
            const CWalletTx &wtx = it->second;
            // Clearly invalid input, fail
            if (wtx.tx->vout.size() <= outpoint.GetN()) {
                return false;
            }
            // Just to calculate the marginal byte size
            CInputCoin coin(wtx.tx, outpoint.GetN(),
                            wtx.GetSpendSize(outpoint.GetN(), false));
            nValueFromPresetInputs += coin.txout.nValue;
            if (coin.m_input_bytes <= 0) {
                // Not solvable, can't estimate size for fee
                return false;
            }
            coin.effective_value =
                coin.txout.nValue -
                coin_selection_params.effective_fee.GetFee(coin.m_input_bytes);
            if (coin_selection_params.use_bnb) {
                value_to_select -= coin.effective_value;
            } else {
                value_to_select -= coin.txout.nValue;
            }
            setPresetCoins.insert(coin);
        } else {
            return false; // TODO: Allow non-wallet inputs
        }
    }

    // Remove preset inputs from vCoins
    for (std::vector<COutput>::iterator it = vCoins.begin();
         it != vCoins.end() && coin_control.HasSelected();) {
        if (setPresetCoins.count(it->GetInputCoin())) {
            it = vCoins.erase(it);
        } else {
            ++it;
        }
    }

    size_t max_ancestors{0};
    size_t max_descendants{0};
    chain().getPackageLimits(max_ancestors, max_descendants);
    bool fRejectLongChains = gArgs.GetBoolArg(
        "-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS);

    // form groups from remaining coins; note that preset coins will not
    // automatically have their associated (same address) coins included
    if (coin_control.m_avoid_partial_spends &&
        vCoins.size() > OUTPUT_GROUP_MAX_ENTRIES) {
        // Cases where we have 11+ outputs all pointing to the same destination
        // may result in privacy leaks as they will potentially be
        // deterministically sorted. We solve that by explicitly shuffling the
        // outputs before processing
        Shuffle(vCoins.begin(), vCoins.end(), FastRandomContext());
    }

    std::vector<OutputGroup> groups = GroupOutputs(
        vCoins, !coin_control.m_avoid_partial_spends, max_ancestors);

    bool res =
        value_to_select <= Amount::zero() ||
        SelectCoinsMinConf(value_to_select, CoinEligibilityFilter(1, 6, 0),
                           groups, setCoinsRet, nValueRet,
                           coin_selection_params, bnb_used) ||
        SelectCoinsMinConf(value_to_select, CoinEligibilityFilter(1, 1, 0),
                           groups, setCoinsRet, nValueRet,
                           coin_selection_params, bnb_used) ||
        (m_spend_zero_conf_change &&
         SelectCoinsMinConf(value_to_select, CoinEligibilityFilter(0, 1, 2),
                            groups, setCoinsRet, nValueRet,
                            coin_selection_params, bnb_used)) ||
        (m_spend_zero_conf_change &&
         SelectCoinsMinConf(
             value_to_select,
             CoinEligibilityFilter(0, 1, std::min((size_t)4, max_ancestors / 3),
                                   std::min((size_t)4, max_descendants / 3)),
             groups, setCoinsRet, nValueRet, coin_selection_params,
             bnb_used)) ||
        (m_spend_zero_conf_change &&
         SelectCoinsMinConf(value_to_select,
                            CoinEligibilityFilter(0, 1, max_ancestors / 2,
                                                  max_descendants / 2),
                            groups, setCoinsRet, nValueRet,
                            coin_selection_params, bnb_used)) ||
        (m_spend_zero_conf_change &&
         SelectCoinsMinConf(value_to_select,
                            CoinEligibilityFilter(0, 1, max_ancestors - 1,
                                                  max_descendants - 1),
                            groups, setCoinsRet, nValueRet,
                            coin_selection_params, bnb_used)) ||
        (m_spend_zero_conf_change && !fRejectLongChains &&
         SelectCoinsMinConf(
             value_to_select,
             CoinEligibilityFilter(0, 1, std::numeric_limits<uint64_t>::max()),
             groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used));

    // Because SelectCoinsMinConf clears the setCoinsRet, we now add the
    // possible inputs to the coinset.
    util::insert(setCoinsRet, setPresetCoins);

    // Add preset inputs to the total value selected.
    nValueRet += nValueFromPresetInputs;

    return res;
}

bool CWallet::SignTransaction(CMutableTransaction &tx) const {
    AssertLockHeld(cs_wallet);

    // Build coins map
    std::map<COutPoint, Coin> coins;
    for (auto &input : tx.vin) {
        auto mi = mapWallet.find(input.prevout.GetTxId());
        if (mi == mapWallet.end() ||
            input.prevout.GetN() >= mi->second.tx->vout.size()) {
            return false;
        }
        const CWalletTx &wtx = mi->second;
        coins[input.prevout] =
            Coin(wtx.tx->vout[input.prevout.GetN()], wtx.m_confirm.block_height,
                 wtx.IsCoinBase());
    }
    std::map<int, std::string> input_errors;
    return SignTransaction(tx, coins, SigHashType().withForkId(), input_errors);
}

bool CWallet::SignTransaction(CMutableTransaction &tx,
                              const std::map<COutPoint, Coin> &coins,
                              SigHashType sighash,
                              std::map<int, std::string> &input_errors) const {
    // Try to sign with all ScriptPubKeyMans
    for (ScriptPubKeyMan *spk_man : GetAllScriptPubKeyMans()) {
        // spk_man->SignTransaction will return true if the transaction is
        // complete, so we can exit early and return true if that happens
        if (spk_man->SignTransaction(tx, coins, sighash, input_errors)) {
            return true;
        }
    }

    // At this point, one input was not fully signed otherwise we would have
    // exited already Find that input and figure out what went wrong.
    for (size_t i = 0; i < tx.vin.size(); i++) {
        // Get the prevout
        CTxIn &txin = tx.vin[i];
        auto coin = coins.find(txin.prevout);
        if (coin == coins.end() || coin->second.IsSpent()) {
            input_errors[i] = "Input not found or already spent";
            continue;
        }

        // Check if this input is complete
        SignatureData sigdata =
            DataFromTransaction(tx, i, coin->second.GetTxOut());
        if (!sigdata.complete) {
            input_errors[i] = "Unable to sign input, missing keys";
            continue;
        }
    }

    // When there are no available providers for the remaining inputs, use the
    // legacy provider so we can get proper error messages.
    auto legacy_spk_man = GetLegacyScriptPubKeyMan();
    if (legacy_spk_man &&
        legacy_spk_man->SignTransaction(tx, coins, sighash, input_errors)) {
        return true;
    }

    return false;
}

TransactionError CWallet::FillPSBT(PartiallySignedTransaction &psbtx,
                                   bool &complete, SigHashType sighash_type,
                                   bool sign, bool bip32derivs) const {
    LOCK(cs_wallet);
    // Get all of the previous transactions
    for (size_t i = 0; i < psbtx.tx->vin.size(); ++i) {
        const CTxIn &txin = psbtx.tx->vin[i];
        PSBTInput &input = psbtx.inputs.at(i);

        if (PSBTInputSigned(input)) {
            continue;
        }

        // If we have no utxo, grab it from the wallet.
        if (input.utxo.IsNull()) {
            const TxId &txid = txin.prevout.GetTxId();
            const auto it = mapWallet.find(txid);
            if (it != mapWallet.end()) {
                const CWalletTx &wtx = it->second;
                CTxOut utxo = wtx.tx->vout[txin.prevout.GetN()];
                // Update UTXOs from the wallet.
                input.utxo = utxo;
            }
        }
    }

    // Fill in information from ScriptPubKeyMans
    for (ScriptPubKeyMan *spk_man : GetAllScriptPubKeyMans()) {
        TransactionError res =
            spk_man->FillPSBT(psbtx, sighash_type, sign, bip32derivs);
        if (res != TransactionError::OK) {
            return res;
        }
    }

    // Complete if every input is now signed
    complete = true;
    for (const auto &input : psbtx.inputs) {
        complete &= PSBTInputSigned(input);
    }

    return TransactionError::OK;
}

SigningResult CWallet::SignMessage(const std::string &message,
                                   const PKHash &pkhash,
                                   std::string &str_sig) const {
    SignatureData sigdata;
    CScript script_pub_key = GetScriptForDestination(pkhash);
    for (const auto &spk_man_pair : m_spk_managers) {
        if (spk_man_pair.second->CanProvide(script_pub_key, sigdata)) {
            return spk_man_pair.second->SignMessage(message, pkhash, str_sig);
        }
    }
    return SigningResult::PRIVATE_KEY_NOT_AVAILABLE;
}

bool CWallet::FundTransaction(CMutableTransaction &tx, Amount &nFeeRet,
                              int &nChangePosInOut, bilingual_str &error,
                              bool lockUnspents,
                              const std::set<int> &setSubtractFeeFromOutputs,
                              CCoinControl coinControl) {
    std::vector<CRecipient> vecSend;

    // Turn the txout set into a CRecipient vector.
    for (size_t idx = 0; idx < tx.vout.size(); idx++) {
        const CTxOut &txOut = tx.vout[idx];
        CRecipient recipient = {txOut.scriptPubKey, txOut.nValue,
                                setSubtractFeeFromOutputs.count(idx) == 1};
        vecSend.push_back(recipient);
    }

    coinControl.fAllowOtherInputs = true;

    for (const CTxIn &txin : tx.vin) {
        coinControl.Select(txin.prevout);
    }

    // Acquire the locks to prevent races to the new locked unspents between the
    // CreateTransaction call and LockCoin calls (when lockUnspents is true).
    LOCK(cs_wallet);

    CTransactionRef tx_new;
    if (!CreateTransaction(vecSend, tx_new, nFeeRet, nChangePosInOut, error,
                           coinControl, false)) {
        return false;
    }

    if (nChangePosInOut != -1) {
        tx.vout.insert(tx.vout.begin() + nChangePosInOut,
                       tx_new->vout[nChangePosInOut]);
    }

    // Copy output sizes from new transaction; they may have had the fee
    // subtracted from them.
    for (size_t idx = 0; idx < tx.vout.size(); idx++) {
        tx.vout[idx].nValue = tx_new->vout[idx].nValue;
    }

    // Add new txins (keeping original txin scriptSig/order)
    for (const CTxIn &txin : tx_new->vin) {
        if (!coinControl.IsSelected(txin.prevout)) {
            tx.vin.push_back(txin);

            if (lockUnspents) {
                LockCoin(txin.prevout);
            }
        }
    }

    return true;
}

static bool IsCurrentForAntiFeeSniping(interfaces::Chain &chain,
                                       const BlockHash &block_hash) {
    if (chain.isInitialBlockDownload()) {
        return false;
    }

    // in seconds
    constexpr int64_t MAX_ANTI_FEE_SNIPING_TIP_AGE = 8 * 60 * 60;
    int64_t block_time;
    CHECK_NONFATAL(chain.findBlock(block_hash, FoundBlock().time(block_time)));
    if (block_time < (GetTime() - MAX_ANTI_FEE_SNIPING_TIP_AGE)) {
        return false;
    }
    return true;
}

/**
 * Return a height-based locktime for new transactions (uses the height of the
 * current chain tip unless we are not synced with the current chain
 */
static uint32_t GetLocktimeForNewTransaction(interfaces::Chain &chain,
                                             const BlockHash &block_hash,
                                             int block_height) {
    uint32_t locktime;
    // Discourage fee sniping.
    //
    // For a large miner the value of the transactions in the best block and
    // the mempool can exceed the cost of deliberately attempting to mine two
    // blocks to orphan the current best block. By setting nLockTime such that
    // only the next block can include the transaction, we discourage this
    // practice as the height restricted and limited blocksize gives miners
    // considering fee sniping fewer options for pulling off this attack.
    //
    // A simple way to think about this is from the wallet's point of view we
    // always want the blockchain to move forward. By setting nLockTime this
    // way we're basically making the statement that we only want this
    // transaction to appear in the next block; we don't want to potentially
    // encourage reorgs by allowing transactions to appear at lower heights
    // than the next block in forks of the best chain.
    //
    // Of course, the subsidy is high enough, and transaction volume low
    // enough, that fee sniping isn't a problem yet, but by implementing a fix
    // now we ensure code won't be written that makes assumptions about
    // nLockTime that preclude a fix later.
    if (IsCurrentForAntiFeeSniping(chain, block_hash)) {
        locktime = block_height;

        // Secondly occasionally randomly pick a nLockTime even further back, so
        // that transactions that are delayed after signing for whatever reason,
        // e.g. high-latency mix networks and some CoinJoin implementations,
        // have better privacy.
        if (GetRandInt(10) == 0) {
            locktime = std::max(0, int(locktime) - GetRandInt(100));
        }
    } else {
        // If our chain is lagging behind, we can't discourage fee sniping nor
        // help the privacy of high-latency transactions. To avoid leaking a
        // potentially unique "nLockTime fingerprint", set nLockTime to a
        // constant.
        locktime = 0;
    }
    assert(locktime < LOCKTIME_THRESHOLD);
    return locktime;
}

OutputType
CWallet::TransactionChangeType(OutputType change_type,
                               const std::vector<CRecipient> &vecSend) {
    // If -changetype is specified, always use that change type.
    if (change_type != OutputType::CHANGE_AUTO) {
        return change_type;
    }

    // if m_default_address_type is legacy, use legacy address as change.
    if (m_default_address_type == OutputType::LEGACY) {
        return OutputType::LEGACY;
    }

    // else use m_default_address_type for change
    return m_default_address_type;
}

bool CWallet::CreateTransactionInternal(const std::vector<CRecipient> &vecSend,
                                        CTransactionRef &tx, Amount &nFeeRet,
                                        int &nChangePosInOut,
                                        bilingual_str &error,
                                        const CCoinControl &coin_control,
                                        bool sign) {
    Amount nValue = Amount::zero();
    const OutputType change_type = TransactionChangeType(
        coin_control.m_change_type ? *coin_control.m_change_type
                                   : m_default_change_type,
        vecSend);
    ReserveDestination reservedest(this, change_type);
    int nChangePosRequest = nChangePosInOut;
    unsigned int nSubtractFeeFromAmount = 0;
    for (const auto &recipient : vecSend) {
        if (nValue < Amount::zero() || recipient.nAmount < Amount::zero()) {
            error = _("Transaction amounts must not be negative");
            return false;
        }

        nValue += recipient.nAmount;

        if (recipient.fSubtractFeeFromAmount) {
            nSubtractFeeFromAmount++;
        }
    }

    if (vecSend.empty()) {
        error = _("Transaction must have at least one recipient");
        return false;
    }

    CMutableTransaction txNew;

    {
        std::set<CInputCoin> setCoins;
        LOCK(cs_wallet);
        txNew.nLockTime = GetLocktimeForNewTransaction(
            chain(), GetLastBlockHash(), GetLastBlockHeight());
        std::vector<COutput> vAvailableCoins;
        AvailableCoins(vAvailableCoins, true, &coin_control);
        // Parameters for coin selection, init with dummy
        CoinSelectionParams coin_selection_params;

        // Create change script that will be used if we need change
        // TODO: pass in scriptChange instead of reservedest so
        // change transaction isn't always pay-to-bitcoin-address
        CScript scriptChange;

        // coin control: send change to custom address
        if (!boost::get<CNoDestination>(&coin_control.destChange)) {
            scriptChange = GetScriptForDestination(coin_control.destChange);

            // no coin control: send change to newly generated address
        } else {
            // Note: We use a new key here to keep it from being obvious
            // which side is the change.
            //  The drawback is that by not reusing a previous key, the
            //  change may be lost if a backup is restored, if the backup
            //  doesn't have the new private key for the change. If we
            //  reused the old key, it would be possible to add code to look
            //  for and rediscover unknown transactions that were written
            //  with keys of ours to recover post-backup change.

            // Reserve a new key pair from key pool. If it fails, provide a
            // dummy destination in case we don't need change.
            CTxDestination dest;
            if (!reservedest.GetReservedDestination(dest, true)) {
                error = _("Transaction needs a change address, but we can't "
                          "generate it. Please call keypoolrefill first.");
            }

            scriptChange = GetScriptForDestination(dest);
            // A valid destination implies a change script (and
            // vice-versa). An empty change script will abort later, if the
            // change keypool ran out, but change is required.
            CHECK_NONFATAL(IsValidDestination(dest) != scriptChange.empty());
        }
        CTxOut change_prototype_txout(Amount::zero(), scriptChange);
        coin_selection_params.change_output_size =
            GetSerializeSize(change_prototype_txout);

        // Get the fee rate to use effective values in coin selection
        CFeeRate nFeeRateNeeded = GetMinimumFeeRate(*this, coin_control);

        nFeeRet = Amount::zero();
        bool pick_new_inputs = true;
        Amount nValueIn = Amount::zero();

        // BnB selector is the only selector used when this is true.
        // That should only happen on the first pass through the loop.
        coin_selection_params.use_bnb = true;
        // If we are doing subtract fee from recipient, don't use effective
        // values
        coin_selection_params.m_subtract_fee_outputs =
            nSubtractFeeFromAmount != 0;
        // Start with no fee and loop until there is enough fee
        while (true) {
            nChangePosInOut = nChangePosRequest;
            txNew.vin.clear();
            txNew.vout.clear();
            bool fFirst = true;

            Amount nValueToSelect = nValue;
            if (nSubtractFeeFromAmount == 0) {
                nValueToSelect += nFeeRet;
            }

            // vouts to the payees
            if (!coin_selection_params.m_subtract_fee_outputs) {
                // Static size overhead + outputs vsize. 4 nVersion, 4
                // nLocktime, 1 input count, 1 output count
                coin_selection_params.tx_noinputs_size = 10;
            }
            // vouts to the payees
            for (const auto &recipient : vecSend) {
                CTxOut txout(recipient.nAmount, recipient.scriptPubKey);

                if (recipient.fSubtractFeeFromAmount) {
                    assert(nSubtractFeeFromAmount != 0);
                    // Subtract fee equally from each selected recipient.
                    txout.nValue -= nFeeRet / int(nSubtractFeeFromAmount);

                    // First receiver pays the remainder not divisible by output
                    // count.
                    if (fFirst) {
                        fFirst = false;
                        txout.nValue -= nFeeRet % int(nSubtractFeeFromAmount);
                    }
                }

                // Include the fee cost for outputs. Note this is only used for
                // BnB right now
                if (!coin_selection_params.m_subtract_fee_outputs) {
                    coin_selection_params.tx_noinputs_size +=
                        ::GetSerializeSize(txout, PROTOCOL_VERSION);
                }

                if (IsDust(txout, chain().relayDustFee())) {
                    if (recipient.fSubtractFeeFromAmount &&
                        nFeeRet > Amount::zero()) {
                        if (txout.nValue < Amount::zero()) {
                            error = _("The transaction amount is too small to "
                                      "pay the fee");
                        } else {
                            error = _("The transaction amount is too small to "
                                      "send after the fee has been deducted");
                        }
                    } else {
                        error = _("Transaction amount too small");
                    }

                    return false;
                }

                txNew.vout.push_back(txout);
            }

            // Choose coins to use
            bool bnb_used = false;
            if (pick_new_inputs) {
                nValueIn = Amount::zero();
                setCoins.clear();
                int change_spend_size = CalculateMaximumSignedInputSize(
                    change_prototype_txout, this);
                // If the wallet doesn't know how to sign change output, assume
                // p2pkh as lower-bound to allow BnB to do it's thing
                if (change_spend_size == -1) {
                    coin_selection_params.change_spend_size =
                        DUMMY_P2PKH_INPUT_SIZE;
                } else {
                    coin_selection_params.change_spend_size =
                        size_t(change_spend_size);
                }
                coin_selection_params.effective_fee = nFeeRateNeeded;
                if (!SelectCoins(vAvailableCoins, nValueToSelect, setCoins,
                                 nValueIn, coin_control, coin_selection_params,
                                 bnb_used)) {
                    // If BnB was used, it was the first pass. No longer the
                    // first pass and continue loop with knapsack.
                    if (bnb_used) {
                        coin_selection_params.use_bnb = false;
                        continue;
                    } else {
                        error = _("Insufficient funds");
                        return false;
                    }
                }
            } else {
                bnb_used = false;
            }

            const Amount nChange = nValueIn - nValueToSelect;
            if (nChange > Amount::zero()) {
                // Fill a vout to ourself.
                CTxOut newTxOut(nChange, scriptChange);

                // Never create dust outputs; if we would, just add the dust to
                // the fee.
                // The nChange when BnB is used is always going to go to fees.
                if (IsDust(newTxOut, chain().relayDustFee()) || bnb_used) {
                    nChangePosInOut = -1;
                    nFeeRet += nChange;
                } else {
                    if (nChangePosInOut == -1) {
                        // Insert change txn at random position:
                        nChangePosInOut = GetRandInt(txNew.vout.size() + 1);
                    } else if ((unsigned int)nChangePosInOut >
                               txNew.vout.size()) {
                        error = _("Change index out of range");
                        return false;
                    }

                    std::vector<CTxOut>::iterator position =
                        txNew.vout.begin() + nChangePosInOut;
                    txNew.vout.insert(position, newTxOut);
                }
            } else {
                nChangePosInOut = -1;
            }

            // Dummy fill vin for maximum size estimation
            //
            for (const auto &coin : setCoins) {
                txNew.vin.push_back(CTxIn(coin.outpoint, CScript()));
            }

            CTransaction txNewConst(txNew);
            int nBytes = CalculateMaximumSignedTxSize(
                txNewConst, this, coin_control.fAllowWatchOnly);
            if (nBytes < 0) {
                error = _("Signing transaction failed");
                return false;
            }

            Amount nFeeNeeded = GetMinimumFee(*this, nBytes, coin_control);

            if (nFeeRet >= nFeeNeeded) {
                // Reduce fee to only the needed amount if possible. This
                // prevents potential overpayment in fees if the coins selected
                // to meet nFeeNeeded result in a transaction that requires less
                // fee than the prior iteration.

                // If we have no change and a big enough excess fee, then try to
                // construct transaction again only without picking new inputs.
                // We now know we only need the smaller fee (because of reduced
                // tx size) and so we should add a change output. Only try this
                // once.
                if (nChangePosInOut == -1 && nSubtractFeeFromAmount == 0 &&
                    pick_new_inputs) {
                    // Add 2 as a buffer in case increasing # of outputs changes
                    // compact size
                    unsigned int tx_size_with_change =
                        nBytes + coin_selection_params.change_output_size + 2;
                    Amount fee_needed_with_change =
                        GetMinimumFee(*this, tx_size_with_change, coin_control);
                    Amount minimum_value_for_change = GetDustThreshold(
                        change_prototype_txout, chain().relayDustFee());
                    if (nFeeRet >=
                        fee_needed_with_change + minimum_value_for_change) {
                        pick_new_inputs = false;
                        nFeeRet = fee_needed_with_change;
                        continue;
                    }
                }

                // If we have change output already, just increase it
                if (nFeeRet > nFeeNeeded && nChangePosInOut != -1 &&
                    nSubtractFeeFromAmount == 0) {
                    Amount extraFeePaid = nFeeRet - nFeeNeeded;
                    std::vector<CTxOut>::iterator change_position =
                        txNew.vout.begin() + nChangePosInOut;
                    change_position->nValue += extraFeePaid;
                    nFeeRet -= extraFeePaid;
                }

                // Done, enough fee included.
                break;
            } else if (!pick_new_inputs) {
                // This shouldn't happen, we should have had enough excess fee
                // to pay for the new output and still meet nFeeNeeded.
                // Or we should have just subtracted fee from recipients and
                // nFeeNeeded should not have changed.
                error = _("Transaction fee and change calculation failed");
                return false;
            }

            // Try to reduce change to include necessary fee.
            if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
                Amount additionalFeeNeeded = nFeeNeeded - nFeeRet;
                std::vector<CTxOut>::iterator change_position =
                    txNew.vout.begin() + nChangePosInOut;
                // Only reduce change if remaining amount is still a large
                // enough output.
                if (change_position->nValue >=
                    MIN_FINAL_CHANGE + additionalFeeNeeded) {
                    change_position->nValue -= additionalFeeNeeded;
                    nFeeRet += additionalFeeNeeded;
                    // Done, able to increase fee from change.
                    break;
                }
            }

            // If subtracting fee from recipients, we now know what fee we
            // need to subtract, we have no reason to reselect inputs.
            if (nSubtractFeeFromAmount > 0) {
                pick_new_inputs = false;
            }

            // Include more fee and try again.
            nFeeRet = nFeeNeeded;
            coin_selection_params.use_bnb = false;
            continue;
        }

        // Give up if change keypool ran out and change is required
        if (scriptChange.empty() && nChangePosInOut != -1) {
            return false;
        }

        // Shuffle selected coins and fill in final vin
        txNew.vin.clear();
        std::vector<CInputCoin> selected_coins(setCoins.begin(),
                                               setCoins.end());
        Shuffle(selected_coins.begin(), selected_coins.end(),
                FastRandomContext());

        // Note how the sequence number is set to non-maxint so that
        // the nLockTime set above actually works.
        for (const auto &coin : selected_coins) {
            txNew.vin.push_back(
                CTxIn(coin.outpoint, CScript(),
                      std::numeric_limits<uint32_t>::max() - 1));
        }

        if (sign && !SignTransaction(txNew)) {
            error = _("Signing transaction failed");
            return false;
        }

        // Return the constructed transaction data.
        tx = MakeTransactionRef(std::move(txNew));

        // Limit size.
        if (tx->GetTotalSize() > MAX_STANDARD_TX_SIZE) {
            error = _("Transaction too large");
            return false;
        }
    }

    if (nFeeRet > m_default_max_tx_fee) {
        error = TransactionErrorString(TransactionError::MAX_FEE_EXCEEDED);
        return false;
    }

    if (gArgs.GetBoolArg("-walletrejectlongchains",
                         DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
        // Lastly, ensure this tx will pass the mempool's chain limits
        if (!chain().checkChainLimits(tx)) {
            error = _("Transaction has too long of a mempool chain");
            return false;
        }
    }

    // Before we return success, we assume any change key will be used to
    // prevent accidental re-use.
    reservedest.KeepDestination();

    return true;
}

bool CWallet::CreateTransaction(const std::vector<CRecipient> &vecSend,
                                CTransactionRef &tx, Amount &nFeeRet,
                                int &nChangePosInOut, bilingual_str &error,
                                const CCoinControl &coin_control, bool sign) {
    int nChangePosIn = nChangePosInOut;
    CTransactionRef tx2 = tx;
    bool res = CreateTransactionInternal(vecSend, tx, nFeeRet, nChangePosInOut,
                                         error, coin_control, sign);
    // try with avoidpartialspends unless it's enabled already
    if (res &&
        nFeeRet >
            Amount::zero() /* 0 means non-functional fee rate estimation */
        && m_max_aps_fee > (-1 * SATOSHI) &&
        !coin_control.m_avoid_partial_spends) {
        CCoinControl tmp_cc = coin_control;
        tmp_cc.m_avoid_partial_spends = true;
        Amount nFeeRet2;
        int nChangePosInOut2 = nChangePosIn;
        // fired and forgotten; if an error occurs, we discard the results
        bilingual_str error2;
        if (CreateTransactionInternal(vecSend, tx2, nFeeRet2, nChangePosInOut2,
                                      error2, tmp_cc, sign)) {
            // if fee of this alternative one is within the range of the max
            // fee, we use this one
            const bool use_aps = nFeeRet2 <= nFeeRet + m_max_aps_fee;
            WalletLogPrintf(
                "Fee non-grouped = %lld, grouped = %lld, using %s\n", nFeeRet,
                nFeeRet2, use_aps ? "grouped" : "non-grouped");
            if (use_aps) {
                tx = tx2;
                nFeeRet = nFeeRet2;
                nChangePosInOut = nChangePosInOut2;
            }
        }
    }
    return res;
}

void CWallet::CommitTransaction(
    CTransactionRef tx, mapValue_t mapValue,
    std::vector<std::pair<std::string, std::string>> orderForm) {
    LOCK(cs_wallet);

    WalletLogPrintfToBeContinued("CommitTransaction:\n%s", tx->ToString());

    // Add tx to wallet, because if it has change it's also ours, otherwise just
    // for transaction history.
    AddToWallet(tx, {}, [&](CWalletTx &wtx, bool new_tx) {
        CHECK_NONFATAL(wtx.mapValue.empty());
        CHECK_NONFATAL(wtx.vOrderForm.empty());
        wtx.mapValue = std::move(mapValue);
        wtx.vOrderForm = std::move(orderForm);
        wtx.fTimeReceivedIsTxTime = true;
        wtx.fFromMe = true;
        return true;
    });

    // Notify that old coins are spent.
    for (const CTxIn &txin : tx->vin) {
        CWalletTx &coin = mapWallet.at(txin.prevout.GetTxId());
        coin.MarkDirty();
        NotifyTransactionChanged(this, coin.GetId(), CT_UPDATED);
    }

    // Get the inserted-CWalletTx from mapWallet so that the
    // fInMempool flag is cached properly
    CWalletTx &wtx = mapWallet.at(tx->GetId());

    if (!fBroadcastTransactions) {
        // Don't submit tx to the mempool
        return;
    }

    std::string err_string;
    if (!wtx.SubmitMemoryPoolAndRelay(err_string, true)) {
        WalletLogPrintf("CommitTransaction(): Transaction cannot be broadcast "
                        "immediately, %s\n",
                        err_string);
        // TODO: if we expect the failure to be long term or permanent, instead
        // delete wtx from the wallet and return failure.
    }
}

DBErrors CWallet::LoadWallet(bool &fFirstRunRet) {
    LOCK(cs_wallet);

    fFirstRunRet = false;
    DBErrors nLoadWalletRet = WalletBatch(*database, "cr+").LoadWallet(this);
    if (nLoadWalletRet == DBErrors::NEED_REWRITE) {
        if (database->Rewrite("\x04pool")) {
            for (const auto &spk_man_pair : m_spk_managers) {
                spk_man_pair.second->RewriteDB();
            }
        }
    }

    // This wallet is in its first run if there are no ScriptPubKeyMans and it
    // isn't blank or no privkeys
    fFirstRunRet = m_spk_managers.empty() &&
                   !IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS) &&
                   !IsWalletFlagSet(WALLET_FLAG_BLANK_WALLET);
    if (fFirstRunRet) {
        assert(m_external_spk_managers.empty());
        assert(m_internal_spk_managers.empty());
    }

    if (nLoadWalletRet != DBErrors::LOAD_OK) {
        return nLoadWalletRet;
    }

    return DBErrors::LOAD_OK;
}

DBErrors CWallet::ZapSelectTx(std::vector<TxId> &txIdsIn,
                              std::vector<TxId> &txIdsOut) {
    AssertLockHeld(cs_wallet);
    DBErrors nZapSelectTxRet =
        WalletBatch(*database, "cr+").ZapSelectTx(txIdsIn, txIdsOut);
    for (const TxId &txid : txIdsOut) {
        const auto &it = mapWallet.find(txid);
        wtxOrdered.erase(it->second.m_it_wtxOrdered);
        mapWallet.erase(it);
        NotifyTransactionChanged(this, txid, CT_DELETED);
    }

    if (nZapSelectTxRet == DBErrors::NEED_REWRITE) {
        if (database->Rewrite("\x04pool")) {
            for (const auto &spk_man_pair : m_spk_managers) {
                spk_man_pair.second->RewriteDB();
            }
        }
    }

    if (nZapSelectTxRet != DBErrors::LOAD_OK) {
        return nZapSelectTxRet;
    }

    MarkDirty();

    return DBErrors::LOAD_OK;
}

DBErrors CWallet::ZapWalletTx(std::list<CWalletTx> &vWtx) {
    DBErrors nZapWalletTxRet = WalletBatch(*database, "cr+").ZapWalletTx(vWtx);
    if (nZapWalletTxRet == DBErrors::NEED_REWRITE) {
        if (database->Rewrite("\x04pool")) {
            for (const auto &spk_man_pair : m_spk_managers) {
                spk_man_pair.second->RewriteDB();
            }
        }
    }

    if (nZapWalletTxRet != DBErrors::LOAD_OK) {
        return nZapWalletTxRet;
    }

    return DBErrors::LOAD_OK;
}

bool CWallet::SetAddressBookWithDB(WalletBatch &batch,
                                   const CTxDestination &address,
                                   const std::string &strName,
                                   const std::string &strPurpose) {
    bool fUpdated = false;
    {
        LOCK(cs_wallet);
        std::map<CTxDestination, CAddressBookData>::iterator mi =
            m_address_book.find(address);
        fUpdated = (mi != m_address_book.end() && !mi->second.IsChange());
        m_address_book[address].SetLabel(strName);
        // Update purpose only if requested.
        if (!strPurpose.empty()) {
            m_address_book[address].purpose = strPurpose;
        }
    }

    NotifyAddressBookChanged(this, address, strName,
                             IsMine(address) != ISMINE_NO, strPurpose,
                             (fUpdated ? CT_UPDATED : CT_NEW));
    if (!strPurpose.empty() && !batch.WritePurpose(address, strPurpose)) {
        return false;
    }
    return batch.WriteName(address, strName);
}

bool CWallet::SetAddressBook(const CTxDestination &address,
                             const std::string &strName,
                             const std::string &strPurpose) {
    WalletBatch batch(*database);
    return SetAddressBookWithDB(batch, address, strName, strPurpose);
}

bool CWallet::DelAddressBook(const CTxDestination &address) {
    // If we want to delete receiving addresses, we need to take care that
    // DestData "used" (and possibly newer DestData) gets preserved (and the
    // "deleted" address transformed into a change entry instead of actually
    // being deleted)
    // NOTE: This isn't a problem for sending addresses because they never have
    // any DestData yet! When adding new DestData, it should be considered here
    // whether to retain or delete it (or move it?).
    if (IsMine(address)) {
        WalletLogPrintf("%s called with IsMine address, NOT SUPPORTED. Please "
                        "report this bug! %s\n",
                        __func__, PACKAGE_BUGREPORT);
        return false;
    }

    {
        LOCK(cs_wallet);

        // Delete destdata tuples associated with address
        for (const std::pair<const std::string, std::string> &item :
             m_address_book[address].destdata) {
            WalletBatch(*database).EraseDestData(address, item.first);
        }
        m_address_book.erase(address);
    }

    NotifyAddressBookChanged(this, address, "", IsMine(address) != ISMINE_NO,
                             "", CT_DELETED);

    WalletBatch(*database).ErasePurpose(address);
    return WalletBatch(*database).EraseName(address);
}

size_t CWallet::KeypoolCountExternalKeys() const {
    AssertLockHeld(cs_wallet);

    unsigned int count = 0;
    for (auto spk_man : GetActiveScriptPubKeyMans()) {
        count += spk_man->KeypoolCountExternalKeys();
    }

    return count;
}

unsigned int CWallet::GetKeyPoolSize() const {
    AssertLockHeld(cs_wallet);

    unsigned int count = 0;
    for (auto spk_man : GetActiveScriptPubKeyMans()) {
        count += spk_man->GetKeyPoolSize();
    }
    return count;
}

bool CWallet::TopUpKeyPool(unsigned int kpSize) {
    LOCK(cs_wallet);
    bool res = true;
    for (auto spk_man : GetActiveScriptPubKeyMans()) {
        res &= spk_man->TopUp(kpSize);
    }
    return res;
}

bool CWallet::GetNewDestination(const OutputType type, const std::string label,
                                CTxDestination &dest, std::string &error) {
    LOCK(cs_wallet);
    error.clear();
    bool result = false;
    auto spk_man = GetScriptPubKeyMan(type, false /* internal */);
    if (spk_man) {
        spk_man->TopUp();
        result = spk_man->GetNewDestination(type, dest, error);
    } else {
        error = strprintf("Error: No %s addresses available.",
                          FormatOutputType(type));
    }
    if (result) {
        SetAddressBook(dest, label, "receive");
    }

    return result;
}

bool CWallet::GetNewChangeDestination(const OutputType type,
                                      CTxDestination &dest,
                                      std::string &error) {
    LOCK(cs_wallet);
    error.clear();

    ReserveDestination reservedest(this, type);
    if (!reservedest.GetReservedDestination(dest, true)) {
        error = _("Error: Keypool ran out, please call keypoolrefill first")
                    .translated;
        return false;
    }

    reservedest.KeepDestination();
    return true;
}

int64_t CWallet::GetOldestKeyPoolTime() const {
    LOCK(cs_wallet);
    int64_t oldestKey = std::numeric_limits<int64_t>::max();
    for (const auto &spk_man_pair : m_spk_managers) {
        oldestKey =
            std::min(oldestKey, spk_man_pair.second->GetOldestKeyPoolTime());
    }
    return oldestKey;
}

void CWallet::MarkDestinationsDirty(
    const std::set<CTxDestination> &destinations) {
    for (auto &entry : mapWallet) {
        CWalletTx &wtx = entry.second;
        if (wtx.m_is_cache_empty) {
            continue;
        }

        for (size_t i = 0; i < wtx.tx->vout.size(); i++) {
            CTxDestination dst;

            if (ExtractDestination(wtx.tx->vout[i].scriptPubKey, dst) &&
                destinations.count(dst)) {
                wtx.MarkDirty();
                break;
            }
        }
    }
}

std::map<CTxDestination, Amount> CWallet::GetAddressBalances() const {
    std::map<CTxDestination, Amount> balances;

    LOCK(cs_wallet);
    std::set<TxId> trusted_parents;
    for (const auto &walletEntry : mapWallet) {
        const CWalletTx &wtx = walletEntry.second;

        if (!wtx.IsTrusted(trusted_parents)) {
            continue;
        }

        if (wtx.IsImmatureCoinBase()) {
            continue;
        }

        int nDepth = wtx.GetDepthInMainChain();
        if (nDepth < (wtx.IsFromMe(ISMINE_ALL) ? 0 : 1)) {
            continue;
        }

        for (uint32_t i = 0; i < wtx.tx->vout.size(); i++) {
            CTxDestination addr;
            if (!IsMine(wtx.tx->vout[i])) {
                continue;
            }

            if (!ExtractDestination(wtx.tx->vout[i].scriptPubKey, addr)) {
                continue;
            }

            Amount n = IsSpent(COutPoint(walletEntry.first, i))
                           ? Amount::zero()
                           : wtx.tx->vout[i].nValue;

            if (!balances.count(addr)) {
                balances[addr] = Amount::zero();
            }
            balances[addr] += n;
        }
    }

    return balances;
}

std::set<std::set<CTxDestination>> CWallet::GetAddressGroupings() const {
    AssertLockHeld(cs_wallet);
    std::set<std::set<CTxDestination>> groupings;
    std::set<CTxDestination> grouping;

    for (const auto &walletEntry : mapWallet) {
        const CWalletTx &wtx = walletEntry.second;

        if (wtx.tx->vin.size() > 0) {
            bool any_mine = false;
            // Group all input addresses with each other.
            for (const auto &txin : wtx.tx->vin) {
                CTxDestination address;
                // If this input isn't mine, ignore it.
                if (!IsMine(txin)) {
                    continue;
                }

                if (!ExtractDestination(mapWallet.at(txin.prevout.GetTxId())
                                            .tx->vout[txin.prevout.GetN()]
                                            .scriptPubKey,
                                        address)) {
                    continue;
                }

                grouping.insert(address);
                any_mine = true;
            }

            // Group change with input addresses.
            if (any_mine) {
                for (const auto &txout : wtx.tx->vout) {
                    if (IsChange(txout)) {
                        CTxDestination txoutAddr;
                        if (!ExtractDestination(txout.scriptPubKey,
                                                txoutAddr)) {
                            continue;
                        }

                        grouping.insert(txoutAddr);
                    }
                }
            }

            if (grouping.size() > 0) {
                groupings.insert(grouping);
                grouping.clear();
            }
        }

        // Group lone addrs by themselves.
        for (const auto &txout : wtx.tx->vout) {
            if (IsMine(txout)) {
                CTxDestination address;
                if (!ExtractDestination(txout.scriptPubKey, address)) {
                    continue;
                }

                grouping.insert(address);
                groupings.insert(grouping);
                grouping.clear();
            }
        }
    }

    // A set of pointers to groups of addresses.
    std::set<std::set<CTxDestination> *> uniqueGroupings;
    // Map addresses to the unique group containing it.
    std::map<CTxDestination, std::set<CTxDestination> *> setmap;
    for (std::set<CTxDestination> _grouping : groupings) {
        // Make a set of all the groups hit by this new group.
        std::set<std::set<CTxDestination> *> hits;
        std::map<CTxDestination, std::set<CTxDestination> *>::iterator it;
        for (const CTxDestination &address : _grouping) {
            if ((it = setmap.find(address)) != setmap.end()) {
                hits.insert((*it).second);
            }
        }

        // Merge all hit groups into a new single group and delete old groups.
        std::set<CTxDestination> *merged =
            new std::set<CTxDestination>(_grouping);
        for (std::set<CTxDestination> *hit : hits) {
            merged->insert(hit->begin(), hit->end());
            uniqueGroupings.erase(hit);
            delete hit;
        }
        uniqueGroupings.insert(merged);

        // Update setmap.
        for (const CTxDestination &element : *merged) {
            setmap[element] = merged;
        }
    }

    std::set<std::set<CTxDestination>> ret;
    for (const std::set<CTxDestination> *uniqueGrouping : uniqueGroupings) {
        ret.insert(*uniqueGrouping);
        delete uniqueGrouping;
    }

    return ret;
}

std::set<CTxDestination>
CWallet::GetLabelAddresses(const std::string &label) const {
    LOCK(cs_wallet);
    std::set<CTxDestination> result;
    for (const std::pair<const CTxDestination, CAddressBookData> &item :
         m_address_book) {
        if (item.second.IsChange()) {
            continue;
        }
        const CTxDestination &address = item.first;
        const std::string &strName = item.second.GetLabel();
        if (strName == label) {
            result.insert(address);
        }
    }

    return result;
}

bool ReserveDestination::GetReservedDestination(CTxDestination &dest,
                                                bool internal) {
    m_spk_man = pwallet->GetScriptPubKeyMan(type, internal);
    if (!m_spk_man) {
        return false;
    }

    if (nIndex == -1) {
        m_spk_man->TopUp();

        CKeyPool keypool;
        if (!m_spk_man->GetReservedDestination(type, internal, address, nIndex,
                                               keypool)) {
            return false;
        }
        fInternal = keypool.fInternal;
    }
    dest = address;
    return true;
}

void ReserveDestination::KeepDestination() {
    if (nIndex != -1) {
        m_spk_man->KeepDestination(nIndex, type);
    }

    nIndex = -1;
    address = CNoDestination();
}

void ReserveDestination::ReturnDestination() {
    if (nIndex != -1) {
        m_spk_man->ReturnDestination(nIndex, fInternal, address);
    }
    nIndex = -1;
    address = CNoDestination();
}

void CWallet::LockCoin(const COutPoint &output) {
    AssertLockHeld(cs_wallet);
    setLockedCoins.insert(output);
}

void CWallet::UnlockCoin(const COutPoint &output) {
    AssertLockHeld(cs_wallet);
    setLockedCoins.erase(output);
}

void CWallet::UnlockAllCoins() {
    AssertLockHeld(cs_wallet);
    setLockedCoins.clear();
}

bool CWallet::IsLockedCoin(const COutPoint &outpoint) const {
    AssertLockHeld(cs_wallet);

    return setLockedCoins.count(outpoint) > 0;
}

void CWallet::ListLockedCoins(std::vector<COutPoint> &vOutpts) const {
    AssertLockHeld(cs_wallet);
    for (COutPoint outpoint : setLockedCoins) {
        vOutpts.push_back(outpoint);
    }
}

/** @} */ // end of Actions

void CWallet::GetKeyBirthTimes(std::map<CKeyID, int64_t> &mapKeyBirth) const {
    AssertLockHeld(cs_wallet);
    mapKeyBirth.clear();

    LegacyScriptPubKeyMan *spk_man = GetLegacyScriptPubKeyMan();
    assert(spk_man != nullptr);
    LOCK(spk_man->cs_KeyStore);

    // Get birth times for keys with metadata.
    for (const auto &entry : spk_man->mapKeyMetadata) {
        if (entry.second.nCreateTime) {
            mapKeyBirth[entry.first] = entry.second.nCreateTime;
        }
    }

    // map in which we'll infer heights of other keys
    std::map<CKeyID, const CWalletTx::Confirmation *> mapKeyFirstBlock;
    CWalletTx::Confirmation max_confirm;
    // the tip can be reorganized; use a 144-block safety margin
    max_confirm.block_height =
        GetLastBlockHeight() > 144 ? GetLastBlockHeight() - 144 : 0;
    CHECK_NONFATAL(chain().findAncestorByHeight(
        GetLastBlockHash(), max_confirm.block_height,
        FoundBlock().hash(max_confirm.hashBlock)));
    for (const CKeyID &keyid : spk_man->GetKeys()) {
        if (mapKeyBirth.count(keyid) == 0) {
            mapKeyFirstBlock[keyid] = &max_confirm;
        }
    }

    // If there are no such keys, we're done.
    if (mapKeyFirstBlock.empty()) {
        return;
    }

    // Find first block that affects those keys, if there are any left.
    for (const auto &entry : mapWallet) {
        // iterate over all wallet transactions...
        const CWalletTx &wtx = entry.second;
        if (wtx.m_confirm.status == CWalletTx::CONFIRMED) {
            // ... which are already in a block
            for (const CTxOut &txout : wtx.tx->vout) {
                // Iterate over all their outputs...
                for (const auto &keyid :
                     GetAffectedKeys(txout.scriptPubKey, *spk_man)) {
                    // ... and all their affected keys.
                    auto rit = mapKeyFirstBlock.find(keyid);
                    if (rit != mapKeyFirstBlock.end() &&
                        wtx.m_confirm.block_height <
                            rit->second->block_height) {
                        rit->second = &wtx.m_confirm;
                    }
                }
            }
        }
    }

    // Extract block timestamps for those keys.
    for (const auto &entry : mapKeyFirstBlock) {
        int64_t block_time;
        CHECK_NONFATAL(chain().findBlock(entry.second->hashBlock,
                                         FoundBlock().time(block_time)));
        // block times can be 2h off
        mapKeyBirth[entry.first] = block_time - TIMESTAMP_WINDOW;
    }
}

/**
 * Compute smart timestamp for a transaction being added to the wallet.
 *
 * Logic:
 * - If sending a transaction, assign its timestamp to the current time.
 * - If receiving a transaction outside a block, assign its timestamp to the
 *   current time.
 * - If receiving a block with a future timestamp, assign all its (not already
 *   known) transactions' timestamps to the current time.
 * - If receiving a block with a past timestamp, before the most recent known
 *   transaction (that we care about), assign all its (not already known)
 *   transactions' timestamps to the same timestamp as that most-recent-known
 *   transaction.
 * - If receiving a block with a past timestamp, but after the most recent known
 *   transaction, assign all its (not already known) transactions' timestamps to
 *   the block time.
 *
 * For more information see CWalletTx::nTimeSmart,
 * https://bitcointalk.org/?topic=54527, or
 * https://github.com/bitcoin/bitcoin/pull/1393.
 */
unsigned int CWallet::ComputeTimeSmart(const CWalletTx &wtx) const {
    unsigned int nTimeSmart = wtx.nTimeReceived;
    if (!wtx.isUnconfirmed() && !wtx.isAbandoned()) {
        int64_t blocktime;
        if (chain().findBlock(wtx.m_confirm.hashBlock,
                              FoundBlock().time(blocktime))) {
            int64_t latestNow = wtx.nTimeReceived;
            int64_t latestEntry = 0;

            // Tolerate times up to the last timestamp in the wallet not more
            // than 5 minutes into the future
            int64_t latestTolerated = latestNow + 300;
            const TxItems &txOrdered = wtxOrdered;
            for (auto it = txOrdered.rbegin(); it != txOrdered.rend(); ++it) {
                CWalletTx *const pwtx = it->second;
                if (pwtx == &wtx) {
                    continue;
                }
                int64_t nSmartTime;
                nSmartTime = pwtx->nTimeSmart;
                if (!nSmartTime) {
                    nSmartTime = pwtx->nTimeReceived;
                }
                if (nSmartTime <= latestTolerated) {
                    latestEntry = nSmartTime;
                    if (nSmartTime > latestNow) {
                        latestNow = nSmartTime;
                    }
                    break;
                }
            }

            nTimeSmart = std::max(latestEntry, std::min(blocktime, latestNow));
        } else {
            WalletLogPrintf("%s: found %s in block %s not in index\n", __func__,
                            wtx.GetId().ToString(),
                            wtx.m_confirm.hashBlock.ToString());
        }
    }
    return nTimeSmart;
}

bool CWallet::AddDestData(WalletBatch &batch, const CTxDestination &dest,
                          const std::string &key, const std::string &value) {
    if (boost::get<CNoDestination>(&dest)) {
        return false;
    }

    m_address_book[dest].destdata.insert(std::make_pair(key, value));
    return batch.WriteDestData(dest, key, value);
}

bool CWallet::EraseDestData(WalletBatch &batch, const CTxDestination &dest,
                            const std::string &key) {
    if (!m_address_book[dest].destdata.erase(key)) {
        return false;
    }

    return batch.EraseDestData(dest, key);
}

void CWallet::LoadDestData(const CTxDestination &dest, const std::string &key,
                           const std::string &value) {
    m_address_book[dest].destdata.insert(std::make_pair(key, value));
}

bool CWallet::GetDestData(const CTxDestination &dest, const std::string &key,
                          std::string *value) const {
    std::map<CTxDestination, CAddressBookData>::const_iterator i =
        m_address_book.find(dest);
    if (i != m_address_book.end()) {
        CAddressBookData::StringMap::const_iterator j =
            i->second.destdata.find(key);
        if (j != i->second.destdata.end()) {
            if (value) {
                *value = j->second;
            }

            return true;
        }
    }
    return false;
}

std::vector<std::string>
CWallet::GetDestValues(const std::string &prefix) const {
    std::vector<std::string> values;
    for (const auto &address : m_address_book) {
        for (const auto &data : address.second.destdata) {
            if (!data.first.compare(0, prefix.size(), prefix)) {
                values.emplace_back(data.second);
            }
        }
    }
    return values;
}

bool CWallet::Verify(const CChainParams &chainParams, interfaces::Chain &chain,
                     const WalletLocation &location,
                     bilingual_str &error_string,
                     std::vector<bilingual_str> &warnings) {
    // Do some checking on wallet path. It should be either a:
    //
    // 1. Path where a directory can be created.
    // 2. Path to an existing directory.
    // 3. Path to a symlink to a directory.
    // 4. For backwards compatibility, the name of a data file in -walletdir.
    LOCK(cs_wallets);
    const fs::path &wallet_path = location.GetPath();
    fs::file_type path_type = fs::symlink_status(wallet_path).type();
    if (!(path_type == fs::file_not_found || path_type == fs::directory_file ||
          (path_type == fs::symlink_file && fs::is_directory(wallet_path)) ||
          (path_type == fs::regular_file &&
           fs::path(location.GetName()).filename() == location.GetName()))) {
        error_string = Untranslated(
            strprintf("Invalid -wallet path '%s'. -wallet path should point to "
                      "a directory where wallet.dat and "
                      "database/log.?????????? files can be stored, a location "
                      "where such a directory could be created, "
                      "or (for backwards compatibility) the name of an "
                      "existing data file in -walletdir (%s)",
                      location.GetName(), GetWalletDir()));
        return false;
    }

    // Make sure that the wallet path doesn't clash with an existing wallet path
    if (IsWalletLoaded(wallet_path)) {
        error_string = Untranslated(strprintf(
            "Error loading wallet %s. Duplicate -wallet filename specified.",
            location.GetName()));
        return false;
    }

    // Keep same database environment instance across Verify/Recover calls
    // below.
    std::unique_ptr<WalletDatabase> database =
        CreateWalletDatabase(wallet_path);

    try {
        return database->Verify(error_string);
    } catch (const fs::filesystem_error &e) {
        error_string = Untranslated(
            strprintf("Error loading wallet %s. %s", location.GetName(),
                      fsbridge::get_filesystem_error_message(e)));
        return false;
    }
}

std::shared_ptr<CWallet> CWallet::CreateWalletFromFile(
    const CChainParams &chainParams, interfaces::Chain &chain,
    const WalletLocation &location, bilingual_str &error,
    std::vector<bilingual_str> &warnings, uint64_t wallet_creation_flags) {
    const std::string walletFile =
        WalletDataFilePath(location.GetPath()).string();

    // Needed to restore wallet transaction meta data after -zapwallettxes
    std::list<CWalletTx> vWtx;

    if (gArgs.GetBoolArg("-zapwallettxes", false)) {
        chain.initMessage(
            _("Zapping all transactions from wallet...").translated);

        std::unique_ptr<CWallet> tempWallet = std::make_unique<CWallet>(
            &chain, location, CreateWalletDatabase(location.GetPath()));
        DBErrors nZapWalletRet = tempWallet->ZapWalletTx(vWtx);
        if (nZapWalletRet != DBErrors::LOAD_OK) {
            error =
                strprintf(_("Error loading %s: Wallet corrupted"), walletFile);
            return nullptr;
        }
    }

    chain.initMessage(_("Loading wallet...").translated);

    int64_t nStart = GetTimeMillis();
    bool fFirstRun = true;
    // TODO: Can't use std::make_shared because we need a custom deleter but
    // should be possible to use std::allocate_shared.
    std::shared_ptr<CWallet> walletInstance(
        new CWallet(&chain, location, CreateWalletDatabase(location.GetPath())),
        ReleaseWallet);
    DBErrors nLoadWalletRet = walletInstance->LoadWallet(fFirstRun);
    if (nLoadWalletRet != DBErrors::LOAD_OK) {
        if (nLoadWalletRet == DBErrors::CORRUPT) {
            error =
                strprintf(_("Error loading %s: Wallet corrupted"), walletFile);
            return nullptr;
        }

        if (nLoadWalletRet == DBErrors::NONCRITICAL_ERROR) {
            warnings.push_back(
                strprintf(_("Error reading %s! All keys read correctly, but "
                            "transaction data or address book entries might be "
                            "missing or incorrect."),
                          walletFile));
        } else if (nLoadWalletRet == DBErrors::TOO_NEW) {
            error = strprintf(
                _("Error loading %s: Wallet requires newer version of %s"),
                walletFile, PACKAGE_NAME);
            return nullptr;
        } else if (nLoadWalletRet == DBErrors::NEED_REWRITE) {
            error = strprintf(
                _("Wallet needed to be rewritten: restart %s to complete"),
                PACKAGE_NAME);
            return nullptr;
        } else {
            error = strprintf(_("Error loading %s"), walletFile);
            return nullptr;
        }
    }

    if (fFirstRun) {
        // Ensure this wallet.dat can only be opened by clients supporting
        // HD with chain split and expects no default key.
        walletInstance->SetMinVersion(FEATURE_LATEST);

        walletInstance->AddWalletFlags(wallet_creation_flags);

        // Only create LegacyScriptPubKeyMan when not descriptor wallet
        if (!walletInstance->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
            walletInstance->SetupLegacyScriptPubKeyMan();
        }

        if (!(wallet_creation_flags &
              (WALLET_FLAG_DISABLE_PRIVATE_KEYS | WALLET_FLAG_BLANK_WALLET))) {
            LOCK(walletInstance->cs_wallet);
            if (walletInstance->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
                walletInstance->SetupDescriptorScriptPubKeyMans();
                // SetupDescriptorScriptPubKeyMans already calls SetupGeneration
                // for us so we don't need to call SetupGeneration separately
            } else {
                // Legacy wallets need SetupGeneration here.
                for (auto spk_man :
                     walletInstance->GetActiveScriptPubKeyMans()) {
                    if (!spk_man->SetupGeneration()) {
                        error = _("Unable to generate initial keys");
                        return nullptr;
                    }
                }
            }
        }

        walletInstance->chainStateFlushed(chain.getTipLocator());
    } else if (wallet_creation_flags & WALLET_FLAG_DISABLE_PRIVATE_KEYS) {
        // Make it impossible to disable private keys after creation
        error = strprintf(_("Error loading %s: Private keys can only be "
                            "disabled during creation"),
                          walletFile);
        return nullptr;
    } else if (walletInstance->IsWalletFlagSet(
                   WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        for (auto spk_man : walletInstance->GetActiveScriptPubKeyMans()) {
            if (spk_man->HavePrivateKeys()) {
                warnings.push_back(
                    strprintf(_("Warning: Private keys detected in wallet {%s} "
                                "with disabled private keys"),
                              walletFile));
            }
        }
    }

    if (gArgs.IsArgSet("-mintxfee")) {
        Amount n = Amount::zero();
        if (!ParseMoney(gArgs.GetArg("-mintxfee", ""), n) ||
            n == Amount::zero()) {
            error = AmountErrMsg("mintxfee", gArgs.GetArg("-mintxfee", ""));
            return nullptr;
        }
        if (n > HIGH_TX_FEE_PER_KB) {
            warnings.push_back(AmountHighWarn("-mintxfee") + Untranslated(" ") +
                               _("This is the minimum transaction fee you pay "
                                 "on every transaction."));
        }
        walletInstance->m_min_fee = CFeeRate(n);
    }

    if (gArgs.IsArgSet("-maxapsfee")) {
        Amount n = Amount::zero();
        if (gArgs.GetArg("-maxapsfee", "") == "-1") {
            n = -1 * SATOSHI;
        } else if (!ParseMoney(gArgs.GetArg("-maxapsfee", ""), n)) {
            error = AmountErrMsg("maxapsfee", gArgs.GetArg("-maxapsfee", ""));
            return nullptr;
        }
        if (n > HIGH_APS_FEE) {
            warnings.push_back(
                AmountHighWarn("-maxapsfee") + Untranslated(" ") +
                _("This is the maximum transaction fee you pay to prioritize "
                  "partial spend avoidance over regular coin selection."));
        }
        walletInstance->m_max_aps_fee = n;
    }

    if (gArgs.IsArgSet("-fallbackfee")) {
        Amount nFeePerK = Amount::zero();
        if (!ParseMoney(gArgs.GetArg("-fallbackfee", ""), nFeePerK)) {
            error =
                strprintf(_("Invalid amount for -fallbackfee=<amount>: '%s'"),
                          gArgs.GetArg("-fallbackfee", ""));
            return nullptr;
        }
        if (nFeePerK > HIGH_TX_FEE_PER_KB) {
            warnings.push_back(AmountHighWarn("-fallbackfee") +
                               Untranslated(" ") +
                               _("This is the transaction fee you may pay when "
                                 "fee estimates are not available."));
        }
        walletInstance->m_fallback_fee = CFeeRate(nFeePerK);
    }
    // Disable fallback fee in case value was set to 0, enable if non-null value
    walletInstance->m_allow_fallback_fee =
        walletInstance->m_fallback_fee.GetFeePerK() != Amount::zero();

    if (gArgs.IsArgSet("-paytxfee")) {
        Amount nFeePerK = Amount::zero();
        if (!ParseMoney(gArgs.GetArg("-paytxfee", ""), nFeePerK)) {
            error = AmountErrMsg("paytxfee", gArgs.GetArg("-paytxfee", ""));
            return nullptr;
        }
        if (nFeePerK > HIGH_TX_FEE_PER_KB) {
            warnings.push_back(AmountHighWarn("-paytxfee") + Untranslated(" ") +
                               _("This is the transaction fee you will pay if "
                                 "you send a transaction."));
        }
        walletInstance->m_pay_tx_fee = CFeeRate(nFeePerK, 1000);
        if (walletInstance->m_pay_tx_fee < chain.relayMinFee()) {
            error = strprintf(_("Invalid amount for -paytxfee=<amount>: '%s' "
                                "(must be at least %s)"),
                              gArgs.GetArg("-paytxfee", ""),
                              chain.relayMinFee().ToString());
            return nullptr;
        }
    }

    if (gArgs.IsArgSet("-maxtxfee")) {
        Amount nMaxFee = Amount::zero();
        if (!ParseMoney(gArgs.GetArg("-maxtxfee", ""), nMaxFee)) {
            error = AmountErrMsg("maxtxfee", gArgs.GetArg("-maxtxfee", ""));
            return nullptr;
        }
        if (nMaxFee > HIGH_MAX_TX_FEE) {
            warnings.push_back(_("-maxtxfee is set very high! Fees this large "
                                 "could be paid on a single transaction."));
        }
        if (CFeeRate(nMaxFee, 1000) < chain.relayMinFee()) {
            error = strprintf(
                _("Invalid amount for -maxtxfee=<amount>: '%s' (must be at "
                  "least the minrelay fee of %s to prevent stuck "
                  "transactions)"),
                gArgs.GetArg("-maxtxfee", ""), chain.relayMinFee().ToString());
            return nullptr;
        }
        walletInstance->m_default_max_tx_fee = nMaxFee;
    }

    if (chain.relayMinFee().GetFeePerK() > HIGH_TX_FEE_PER_KB) {
        warnings.push_back(
            AmountHighWarn("-minrelaytxfee") + Untranslated(" ") +
            _("The wallet will avoid paying less than the minimum relay fee."));
    }

    walletInstance->m_spend_zero_conf_change =
        gArgs.GetBoolArg("-spendzeroconfchange", DEFAULT_SPEND_ZEROCONF_CHANGE);

    walletInstance->m_default_address_type = DEFAULT_ADDRESS_TYPE;
    walletInstance->m_default_change_type = DEFAULT_CHANGE_TYPE;

    walletInstance->WalletLogPrintf("Wallet completed loading in %15dms\n",
                                    GetTimeMillis() - nStart);

    // Try to top up keypool. No-op if the wallet is locked.
    walletInstance->TopUpKeyPool();

    LOCK(walletInstance->cs_wallet);

    // Register wallet with validationinterface. It's done before rescan to
    // avoid missing block connections between end of rescan and validation
    // subscribing. Because of wallet lock being hold, block connection
    // notifications are going to be pending on the validation-side until lock
    // release. It's likely to have block processing duplicata (if rescan block
    // range overlaps with notification one) but we guarantee at least than
    // wallet state is correct after notifications delivery. This is temporary
    // until rescan and notifications delivery are unified under same interface.
    walletInstance->m_chain_notifications_handler =
        walletInstance->chain().handleNotifications(walletInstance);

    int rescan_height = 0;
    if (!gArgs.GetBoolArg("-rescan", false)) {
        WalletBatch batch(*walletInstance->database);
        CBlockLocator locator;
        if (batch.ReadBestBlock(locator)) {
            if (const std::optional<int> fork_height =
                    chain.findLocatorFork(locator)) {
                rescan_height = *fork_height;
            }
        }
    }

    const std::optional<int> tip_height = chain.getHeight();
    if (tip_height) {
        walletInstance->m_last_block_processed =
            chain.getBlockHash(*tip_height);
        walletInstance->m_last_block_processed_height = *tip_height;
    } else {
        walletInstance->m_last_block_processed.SetNull();
        walletInstance->m_last_block_processed_height = -1;
    }

    if (tip_height && *tip_height != rescan_height) {
        // We can't rescan beyond non-pruned blocks, stop and throw an error.
        // This might happen if a user uses an old wallet within a pruned node
        // or if they ran -disablewallet for a longer time, then decided to
        // re-enable
        if (chain.havePruned()) {
            // Exit early and print an error.
            // If a block is pruned after this check, we will load the wallet,
            // but fail the rescan with a generic error.
            int block_height = *tip_height;
            while (block_height > 0 &&
                   chain.haveBlockOnDisk(block_height - 1) &&
                   rescan_height != block_height) {
                --block_height;
            }

            if (rescan_height != block_height) {
                error = _("Prune: last wallet synchronisation goes beyond "
                          "pruned data. You need to -reindex (download the "
                          "whole blockchain again in case of pruned node)");
                return nullptr;
            }
        }

        chain.initMessage(_("Rescanning...").translated);
        walletInstance->WalletLogPrintf(
            "Rescanning last %i blocks (from block %i)...\n",
            *tip_height - rescan_height, rescan_height);

        // No need to read and scan block if block was created before our wallet
        // birthday (as adjusted for block time variability)
        std::optional<int64_t> time_first_key;
        for (auto spk_man : walletInstance->GetAllScriptPubKeyMans()) {
            int64_t time = spk_man->GetTimeFirstKey();
            if (!time_first_key || time < *time_first_key) {
                time_first_key = time;
            }
        }
        if (time_first_key) {
            if (std::optional<int> first_block =
                    chain.findFirstBlockWithTimeAndHeight(
                        *time_first_key - TIMESTAMP_WINDOW, rescan_height,
                        nullptr)) {
                rescan_height = *first_block;
            }
        }

        {
            WalletRescanReserver reserver(*walletInstance);
            if (!reserver.reserve() ||
                (ScanResult::SUCCESS !=
                 walletInstance
                     ->ScanForWalletTransactions(
                         chain.getBlockHash(rescan_height), rescan_height,
                         {} /* max height */, reserver, true /* update */)
                     .status)) {
                error = _("Failed to rescan the wallet during initialization");
                return nullptr;
            }
        }
        walletInstance->chainStateFlushed(chain.getTipLocator());
        walletInstance->database->IncrementUpdateCounter();

        // Restore wallet transaction metadata after -zapwallettxes=1
        if (gArgs.GetBoolArg("-zapwallettxes", false) &&
            gArgs.GetArg("-zapwallettxes", "1") != "2") {
            WalletBatch batch(*walletInstance->database);

            for (const CWalletTx &wtxOld : vWtx) {
                const TxId txid = wtxOld.GetId();
                std::map<TxId, CWalletTx>::iterator mi =
                    walletInstance->mapWallet.find(txid);
                if (mi != walletInstance->mapWallet.end()) {
                    const CWalletTx *copyFrom = &wtxOld;
                    CWalletTx *copyTo = &mi->second;
                    copyTo->mapValue = copyFrom->mapValue;
                    copyTo->vOrderForm = copyFrom->vOrderForm;
                    copyTo->nTimeReceived = copyFrom->nTimeReceived;
                    copyTo->nTimeSmart = copyFrom->nTimeSmart;
                    copyTo->fFromMe = copyFrom->fFromMe;
                    copyTo->nOrderPos = copyFrom->nOrderPos;
                    batch.WriteTx(*copyTo);
                }
            }
        }
    }

    {
        LOCK(cs_wallets);
        for (auto &load_wallet : g_load_wallet_fns) {
            load_wallet(interfaces::MakeWallet(walletInstance));
        }
    }

    walletInstance->SetBroadcastTransactions(
        gArgs.GetBoolArg("-walletbroadcast", DEFAULT_WALLETBROADCAST));

    walletInstance->WalletLogPrintf("setKeyPool.size() = %u\n",
                                    walletInstance->GetKeyPoolSize());
    walletInstance->WalletLogPrintf("mapWallet.size() = %u\n",
                                    walletInstance->mapWallet.size());
    walletInstance->WalletLogPrintf("m_address_book.size() = %u\n",
                                    walletInstance->m_address_book.size());

    return walletInstance;
}

const CAddressBookData *
CWallet::FindAddressBookEntry(const CTxDestination &dest,
                              bool allow_change) const {
    const auto &address_book_it = m_address_book.find(dest);
    if (address_book_it == m_address_book.end()) {
        return nullptr;
    }
    if ((!allow_change) && address_book_it->second.IsChange()) {
        return nullptr;
    }
    return &address_book_it->second;
}

bool CWallet::UpgradeWallet(int version, bilingual_str &error,
                            std::vector<bilingual_str> &warnings) {
    int prev_version = GetVersion();
    int nMaxVersion = version;
    // The -upgradewallet without argument case
    if (nMaxVersion == 0) {
        WalletLogPrintf("Performing wallet upgrade to %i\n", FEATURE_LATEST);
        nMaxVersion = FEATURE_LATEST;
        // permanently upgrade the wallet immediately
        SetMinVersion(FEATURE_LATEST);
    } else {
        WalletLogPrintf("Allowing wallet upgrade up to %i\n", nMaxVersion);
    }

    if (nMaxVersion < GetVersion()) {
        error = _("Cannot downgrade wallet");
        return false;
    }

    SetMaxVersion(nMaxVersion);

    LOCK(cs_wallet);

    // Do not upgrade versions to any version between HD_SPLIT and
    // FEATURE_PRE_SPLIT_KEYPOOL unless already supporting HD_SPLIT
    int max_version = GetVersion();
    if (!CanSupportFeature(FEATURE_HD_SPLIT) &&
        max_version >= FEATURE_HD_SPLIT &&
        max_version < FEATURE_PRE_SPLIT_KEYPOOL) {
        error = _("Cannot upgrade a non HD split wallet without upgrading to "
                  "support pre split keypool. Please use version 200300 or no "
                  "version specified.");
        return false;
    }

    for (auto spk_man : GetActiveScriptPubKeyMans()) {
        if (!spk_man->Upgrade(prev_version, error)) {
            return false;
        }
    }

    return true;
}

void CWallet::postInitProcess() {
    LOCK(cs_wallet);

    // Add wallet transactions that aren't already in a block to mempool.
    // Do this here as mempool requires genesis block to be loaded.
    ReacceptWalletTransactions();

    // Update wallet transactions with current mempool transactions.
    chain().requestMempoolTransactions(*this);
}

bool CWallet::BackupWallet(const std::string &strDest) const {
    return database->Backup(strDest);
}

CKeyPool::CKeyPool() {
    nTime = GetTime();
    fInternal = false;
    m_pre_split = false;
}

CKeyPool::CKeyPool(const CPubKey &vchPubKeyIn, bool internalIn) {
    nTime = GetTime();
    vchPubKey = vchPubKeyIn;
    fInternal = internalIn;
    m_pre_split = false;
}

int CWalletTx::GetDepthInMainChain() const {
    assert(pwallet != nullptr);
    AssertLockHeld(pwallet->cs_wallet);
    if (isUnconfirmed() || isAbandoned()) {
        return 0;
    }

    return (pwallet->GetLastBlockHeight() - m_confirm.block_height + 1) *
           (isConflicted() ? -1 : 1);
}

int CWalletTx::GetBlocksToMaturity() const {
    if (!IsCoinBase()) {
        return 0;
    }

    int chain_depth = GetDepthInMainChain();
    // coinbase tx should not be conflicted
    assert(chain_depth >= 0);
    return std::max(0, (COINBASE_MATURITY + 1) - chain_depth);
}

bool CWalletTx::IsImmatureCoinBase() const {
    // note GetBlocksToMaturity is 0 for non-coinbase tx
    return GetBlocksToMaturity() > 0;
}

std::vector<OutputGroup>
CWallet::GroupOutputs(const std::vector<COutput> &outputs, bool single_coin,
                      const size_t max_ancestors) const {
    std::vector<OutputGroup> groups;
    std::map<CTxDestination, OutputGroup> gmap;
    std::set<CTxDestination> full_groups;

    for (const auto &output : outputs) {
        if (output.fSpendable) {
            CTxDestination dst;
            CInputCoin input_coin = output.GetInputCoin();

            size_t ancestors, descendants;
            chain().getTransactionAncestry(output.tx->GetId(), ancestors,
                                           descendants);
            if (!single_coin &&
                ExtractDestination(output.tx->tx->vout[output.i].scriptPubKey,
                                   dst)) {
                auto it = gmap.find(dst);
                if (it != gmap.end()) {
                    // Limit output groups to no more than
                    // OUTPUT_GROUP_MAX_ENTRIES number of entries, to protect
                    // against inadvertently creating a too-large transaction
                    // when using -avoidpartialspends to prevent breaking
                    // consensus or surprising users with a very high amount of
                    // fees.
                    if (it->second.m_outputs.size() >=
                        OUTPUT_GROUP_MAX_ENTRIES) {
                        groups.push_back(it->second);
                        it->second = OutputGroup{};
                        full_groups.insert(dst);
                    }
                    it->second.Insert(input_coin, output.nDepth,
                                      output.tx->IsFromMe(ISMINE_ALL),
                                      ancestors, descendants);
                } else {
                    gmap[dst].Insert(input_coin, output.nDepth,
                                     output.tx->IsFromMe(ISMINE_ALL), ancestors,
                                     descendants);
                }
            } else {
                groups.emplace_back(input_coin, output.nDepth,
                                    output.tx->IsFromMe(ISMINE_ALL), ancestors,
                                    descendants);
            }
        }
    }
    if (!single_coin) {
        for (auto &it : gmap) {
            auto &group = it.second;
            if (full_groups.count(it.first) > 0) {
                // Make this unattractive as we want coin selection to avoid it
                // if possible
                group.m_ancestors = max_ancestors - 1;
            }
            groups.push_back(group);
        }
    }
    return groups;
}

bool CWallet::IsCrypted() const {
    return HasEncryptionKeys();
}

bool CWallet::IsLocked() const {
    if (!IsCrypted()) {
        return false;
    }
    LOCK(cs_wallet);
    return vMasterKey.empty();
}

bool CWallet::Lock() {
    if (!IsCrypted()) {
        return false;
    }

    {
        LOCK(cs_wallet);
        vMasterKey.clear();
    }

    NotifyStatusChanged(this);
    return true;
}

bool CWallet::Unlock(const CKeyingMaterial &vMasterKeyIn, bool accept_no_keys) {
    {
        LOCK(cs_wallet);
        for (const auto &spk_man_pair : m_spk_managers) {
            if (!spk_man_pair.second->CheckDecryptionKey(vMasterKeyIn,
                                                         accept_no_keys)) {
                return false;
            }
        }
        vMasterKey = vMasterKeyIn;
    }
    NotifyStatusChanged(this);
    return true;
}

std::set<ScriptPubKeyMan *> CWallet::GetActiveScriptPubKeyMans() const {
    std::set<ScriptPubKeyMan *> spk_mans;
    for (bool internal : {false, true}) {
        for (OutputType t : OUTPUT_TYPES) {
            auto spk_man = GetScriptPubKeyMan(t, internal);
            if (spk_man) {
                spk_mans.insert(spk_man);
            }
        }
    }
    return spk_mans;
}

std::set<ScriptPubKeyMan *> CWallet::GetAllScriptPubKeyMans() const {
    std::set<ScriptPubKeyMan *> spk_mans;
    for (const auto &spk_man_pair : m_spk_managers) {
        spk_mans.insert(spk_man_pair.second.get());
    }
    return spk_mans;
}

ScriptPubKeyMan *CWallet::GetScriptPubKeyMan(const OutputType &type,
                                             bool internal) const {
    const std::map<OutputType, ScriptPubKeyMan *> &spk_managers =
        internal ? m_internal_spk_managers : m_external_spk_managers;
    std::map<OutputType, ScriptPubKeyMan *>::const_iterator it =
        spk_managers.find(type);
    if (it == spk_managers.end()) {
        WalletLogPrintf(
            "%s scriptPubKey Manager for output type %d does not exist\n",
            internal ? "Internal" : "External", static_cast<int>(type));
        return nullptr;
    }
    return it->second;
}

std::set<ScriptPubKeyMan *>
CWallet::GetScriptPubKeyMans(const CScript &script,
                             SignatureData &sigdata) const {
    std::set<ScriptPubKeyMan *> spk_mans;
    for (const auto &spk_man_pair : m_spk_managers) {
        if (spk_man_pair.second->CanProvide(script, sigdata)) {
            spk_mans.insert(spk_man_pair.second.get());
        }
    }
    return spk_mans;
}

ScriptPubKeyMan *CWallet::GetScriptPubKeyMan(const CScript &script) const {
    SignatureData sigdata;
    for (const auto &spk_man_pair : m_spk_managers) {
        if (spk_man_pair.second->CanProvide(script, sigdata)) {
            return spk_man_pair.second.get();
        }
    }
    return nullptr;
}

ScriptPubKeyMan *CWallet::GetScriptPubKeyMan(const uint256 &id) const {
    if (m_spk_managers.count(id) > 0) {
        return m_spk_managers.at(id).get();
    }
    return nullptr;
}

std::unique_ptr<SigningProvider>
CWallet::GetSolvingProvider(const CScript &script) const {
    SignatureData sigdata;
    return GetSolvingProvider(script, sigdata);
}

std::unique_ptr<SigningProvider>
CWallet::GetSolvingProvider(const CScript &script,
                            SignatureData &sigdata) const {
    for (const auto &spk_man_pair : m_spk_managers) {
        if (spk_man_pair.second->CanProvide(script, sigdata)) {
            return spk_man_pair.second->GetSolvingProvider(script);
        }
    }
    return nullptr;
}

LegacyScriptPubKeyMan *CWallet::GetLegacyScriptPubKeyMan() const {
    if (IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
        return nullptr;
    }
    // Legacy wallets only have one ScriptPubKeyMan which is a
    // LegacyScriptPubKeyMan. Everything in m_internal_spk_managers and
    // m_external_spk_managers point to the same legacyScriptPubKeyMan.
    auto it = m_internal_spk_managers.find(OutputType::LEGACY);
    if (it == m_internal_spk_managers.end()) {
        return nullptr;
    }
    return dynamic_cast<LegacyScriptPubKeyMan *>(it->second);
}

LegacyScriptPubKeyMan *CWallet::GetOrCreateLegacyScriptPubKeyMan() {
    SetupLegacyScriptPubKeyMan();
    return GetLegacyScriptPubKeyMan();
}

void CWallet::SetupLegacyScriptPubKeyMan() {
    if (!m_internal_spk_managers.empty() || !m_external_spk_managers.empty() ||
        !m_spk_managers.empty() || IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
        return;
    }

    auto spk_manager =
        std::unique_ptr<ScriptPubKeyMan>(new LegacyScriptPubKeyMan(*this));
    for (const auto &type : OUTPUT_TYPES) {
        m_internal_spk_managers[type] = spk_manager.get();
        m_external_spk_managers[type] = spk_manager.get();
    }
    m_spk_managers[spk_manager->GetID()] = std::move(spk_manager);
}

const CKeyingMaterial &CWallet::GetEncryptionKey() const {
    return vMasterKey;
}

bool CWallet::HasEncryptionKeys() const {
    return !mapMasterKeys.empty();
}

void CWallet::ConnectScriptPubKeyManNotifiers() {
    for (const auto &spk_man : GetActiveScriptPubKeyMans()) {
        spk_man->NotifyWatchonlyChanged.connect(NotifyWatchonlyChanged);
        spk_man->NotifyCanGetAddressesChanged.connect(
            NotifyCanGetAddressesChanged);
    }
}

void CWallet::LoadDescriptorScriptPubKeyMan(uint256 id,
                                            WalletDescriptor &desc) {
    auto spk_manager = std::unique_ptr<ScriptPubKeyMan>(
        new DescriptorScriptPubKeyMan(*this, desc));
    m_spk_managers[id] = std::move(spk_manager);
}

void CWallet::SetupDescriptorScriptPubKeyMans() {
    AssertLockHeld(cs_wallet);

    // Make a seed
    CKey seed_key;
    seed_key.MakeNewKey(true);
    CPubKey seed = seed_key.GetPubKey();
    assert(seed_key.VerifyPubKey(seed));

    // Get the extended key
    CExtKey master_key;
    master_key.SetSeed(seed_key.begin(), seed_key.size());

    for (bool internal : {false, true}) {
        for (OutputType t : OUTPUT_TYPES) {
            auto spk_manager =
                std::make_unique<DescriptorScriptPubKeyMan>(*this, internal);
            if (IsCrypted()) {
                if (IsLocked()) {
                    throw std::runtime_error(
                        std::string(__func__) +
                        ": Wallet is locked, cannot setup new descriptors");
                }
                if (!spk_manager->CheckDecryptionKey(vMasterKey) &&
                    !spk_manager->Encrypt(vMasterKey, nullptr)) {
                    throw std::runtime_error(
                        std::string(__func__) +
                        ": Could not encrypt new descriptors");
                }
            }
            spk_manager->SetupDescriptorGeneration(master_key, t);
            uint256 id = spk_manager->GetID();
            m_spk_managers[id] = std::move(spk_manager);
            AddActiveScriptPubKeyMan(id, t, internal);
        }
    }
}

void CWallet::AddActiveScriptPubKeyMan(uint256 id, OutputType type,
                                       bool internal) {
    WalletBatch batch(*database);
    if (!batch.WriteActiveScriptPubKeyMan(static_cast<uint8_t>(type), id,
                                          internal)) {
        throw std::runtime_error(std::string(__func__) +
                                 ": writing active ScriptPubKeyMan id failed");
    }
    LoadActiveScriptPubKeyMan(id, type, internal);
}

void CWallet::LoadActiveScriptPubKeyMan(uint256 id, OutputType type,
                                        bool internal) {
    WalletLogPrintf(
        "Setting spkMan to active: id = %s, type = %d, internal = %d\n",
        id.ToString(), static_cast<int>(type), static_cast<int>(internal));
    auto &spk_mans =
        internal ? m_internal_spk_managers : m_external_spk_managers;
    auto spk_man = m_spk_managers.at(id).get();
    spk_man->SetInternal(internal);
    spk_mans[type] = spk_man;

    NotifyCanGetAddressesChanged();
}

bool CWallet::IsLegacy() const {
    if (m_internal_spk_managers.count(OutputType::LEGACY) == 0) {
        return false;
    }
    auto spk_man = dynamic_cast<LegacyScriptPubKeyMan *>(
        m_internal_spk_managers.at(OutputType::LEGACY));
    return spk_man != nullptr;
}

DescriptorScriptPubKeyMan *
CWallet::GetDescriptorScriptPubKeyMan(const WalletDescriptor &desc) const {
    for (auto &spk_man_pair : m_spk_managers) {
        // Try to downcast to DescriptorScriptPubKeyMan then check if the
        // descriptors match
        DescriptorScriptPubKeyMan *spk_manager =
            dynamic_cast<DescriptorScriptPubKeyMan *>(
                spk_man_pair.second.get());
        if (spk_manager != nullptr && spk_manager->HasWalletDescriptor(desc)) {
            return spk_manager;
        }
    }

    return nullptr;
}

ScriptPubKeyMan *
CWallet::AddWalletDescriptor(WalletDescriptor &desc,
                             const FlatSigningProvider &signing_provider,
                             const std::string &label) {
    if (!IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
        WalletLogPrintf(
            "Cannot add WalletDescriptor to a non-descriptor wallet\n");
        return nullptr;
    }

    LOCK(cs_wallet);
    auto new_spk_man = std::make_unique<DescriptorScriptPubKeyMan>(*this, desc);

    // If we already have this descriptor, remove it from the maps but add the
    // existing cache to desc
    auto old_spk_man = GetDescriptorScriptPubKeyMan(desc);
    if (old_spk_man) {
        WalletLogPrintf("Update existing descriptor: %s\n",
                        desc.descriptor->ToString());

        {
            LOCK(old_spk_man->cs_desc_man);
            new_spk_man->SetCache(old_spk_man->GetWalletDescriptor().cache);
        }

        // Remove from maps of active spkMans
        auto old_spk_man_id = old_spk_man->GetID();
        for (bool internal : {false, true}) {
            for (OutputType t : OUTPUT_TYPES) {
                auto active_spk_man = GetScriptPubKeyMan(t, internal);
                if (active_spk_man &&
                    active_spk_man->GetID() == old_spk_man_id) {
                    if (internal) {
                        m_internal_spk_managers.erase(t);
                    } else {
                        m_external_spk_managers.erase(t);
                    }
                    break;
                }
            }
        }
        m_spk_managers.erase(old_spk_man_id);
    }

    // Add the private keys to the descriptor
    for (const auto &entry : signing_provider.keys) {
        const CKey &key = entry.second;
        new_spk_man->AddDescriptorKey(key, key.GetPubKey());
    }

    // Top up key pool, the manager will generate new scriptPubKeys internally
    new_spk_man->TopUp();

    // Apply the label if necessary
    // Note: we disable labels for ranged descriptors
    if (!desc.descriptor->IsRange()) {
        auto script_pub_keys = new_spk_man->GetScriptPubKeys();
        if (script_pub_keys.empty()) {
            WalletLogPrintf(
                "Could not generate scriptPubKeys (cache is empty)\n");
            return nullptr;
        }

        CTxDestination dest;
        if (ExtractDestination(script_pub_keys.at(0), dest)) {
            SetAddressBook(dest, label, "receive");
        }
    }

    // Save the descriptor to memory
    auto ret = new_spk_man.get();
    m_spk_managers[new_spk_man->GetID()] = std::move(new_spk_man);

    // Save the descriptor to DB
    ret->WriteDescriptor();

    return ret;
}
