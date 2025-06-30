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

#ifndef BITCOIN_TEST_UTIL_TRANSACTION_UTILS_H
#define BITCOIN_TEST_UTIL_TRANSACTION_UTILS_H

#include <primitives/transaction.h>

#include <array>

class FillableSigningProvider;
class CCoinsViewCache;

// create crediting transaction
// [1 coinbase input => 1 output with given scriptPubkey and value]
CMutableTransaction BuildCreditingTransaction(const CScript &scriptPubKey,
                                              const Amount nValue);

// create spending transaction
// [1 input with referenced transaction outpoint, scriptSig =>
//  1 output with empty scriptPubKey, full value of referenced transaction]
CMutableTransaction BuildSpendingTransaction(const CScript &scriptSig,
                                             const CTransaction &txCredit);

// Helper: create two dummy transactions, each with two outputs.
// The first has nValues[0] and nValues[1] outputs paid to a TxoutType::PUBKEY,
// the second nValues[2] and nValues[3] outputs paid to a TxoutType::PUBKEYHASH.
std::vector<CMutableTransaction>
SetupDummyInputs(FillableSigningProvider &keystoreRet,
                 CCoinsViewCache &coinsRet,
                 const std::array<Amount, 4> &nValues);

#endif // BITCOIN_TEST_UTIL_TRANSACTION_UTILS_H
