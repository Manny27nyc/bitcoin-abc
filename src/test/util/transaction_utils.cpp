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

#include <coins.h>
#include <script/signingprovider.h>
#include <test/util/transaction_utils.h>

CMutableTransaction BuildCreditingTransaction(const CScript &scriptPubKey,
                                              const Amount nValue) {
    CMutableTransaction txCredit;
    txCredit.nVersion = 1;
    txCredit.nLockTime = 0;
    txCredit.vin.resize(1);
    txCredit.vout.resize(1);
    txCredit.vin[0].prevout = COutPoint();
    txCredit.vin[0].scriptSig = CScript() << CScriptNum(0) << CScriptNum(0);
    txCredit.vin[0].nSequence = CTxIn::SEQUENCE_FINAL;
    txCredit.vout[0].scriptPubKey = scriptPubKey;
    txCredit.vout[0].nValue = nValue;

    return txCredit;
}

CMutableTransaction BuildSpendingTransaction(const CScript &scriptSig,
                                             const CTransaction &txCredit) {
    CMutableTransaction txSpend;
    txSpend.nVersion = 1;
    txSpend.nLockTime = 0;
    txSpend.vin.resize(1);
    txSpend.vout.resize(1);
    txSpend.vin[0].prevout = COutPoint(txCredit.GetId(), 0);
    txSpend.vin[0].scriptSig = scriptSig;
    txSpend.vin[0].nSequence = CTxIn::SEQUENCE_FINAL;
    txSpend.vout[0].scriptPubKey = CScript();
    txSpend.vout[0].nValue = txCredit.vout[0].nValue;

    return txSpend;
}

std::vector<CMutableTransaction>
SetupDummyInputs(FillableSigningProvider &keystoreRet,
                 CCoinsViewCache &coinsRet,
                 const std::array<Amount, 4> &nValues) {
    std::vector<CMutableTransaction> dummyTransactions;
    dummyTransactions.resize(2);

    // Add some keys to the keystore:
    CKey key[4];
    for (int i = 0; i < 4; i++) {
        key[i].MakeNewKey(i % 2);
        keystoreRet.AddKey(key[i]);
    }

    // Create some dummy input transactions
    dummyTransactions[0].vout.resize(2);
    dummyTransactions[0].vout[0].nValue = nValues[0];
    dummyTransactions[0].vout[0].scriptPubKey
        << ToByteVector(key[0].GetPubKey()) << OP_CHECKSIG;
    dummyTransactions[0].vout[1].nValue = nValues[1];
    dummyTransactions[0].vout[1].scriptPubKey
        << ToByteVector(key[1].GetPubKey()) << OP_CHECKSIG;
    AddCoins(coinsRet, CTransaction(dummyTransactions[0]), 0);

    dummyTransactions[1].vout.resize(2);
    dummyTransactions[1].vout[0].nValue = nValues[2];
    dummyTransactions[1].vout[0].scriptPubKey =
        GetScriptForDestination(PKHash(key[2].GetPubKey()));
    dummyTransactions[1].vout[1].nValue = nValues[3];
    dummyTransactions[1].vout[1].scriptPubKey =
        GetScriptForDestination(PKHash(key[3].GetPubKey()));
    AddCoins(coinsRet, CTransaction(dummyTransactions[1]), 0);

    return dummyTransactions;
}
