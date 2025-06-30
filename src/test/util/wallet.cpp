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
// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/wallet.h>

#include <config.h>
#include <key_io.h>

#ifdef ENABLE_WALLET
#include <wallet/wallet.h>
#endif // ENABLE_WALLET

const std::string ADDRESS_BCHREG_UNSPENDABLE =
    "ecregtest:qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqcrl5mqkt";

#ifdef ENABLE_WALLET
std::string getnewaddress(const Config &config, CWallet &w) {
    constexpr auto output_type = OutputType::LEGACY;
    CTxDestination dest;
    std::string error;
    if (!w.GetNewDestination(output_type, "", dest, error)) {
        assert(false);
    }

    return EncodeDestination(dest, config);
}

void importaddress(CWallet &wallet, const std::string &address) {
    auto spk_man = wallet.GetLegacyScriptPubKeyMan();
    LOCK2(wallet.cs_wallet, spk_man->cs_KeyStore);
    const auto dest = DecodeDestination(address, wallet.GetChainParams());
    assert(IsValidDestination(dest));
    const auto script = GetScriptForDestination(dest);
    wallet.MarkDirty();
    assert(!spk_man->HaveWatchOnly(script));
    if (!spk_man->AddWatchOnly(script, 0 /* nCreateTime */)) {
        assert(false);
    }
    wallet.SetAddressBook(dest, /* label */ "", "receive");
}
#endif // ENABLE_WALLET
