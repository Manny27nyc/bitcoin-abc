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
// Copyright (c) 2019-2020 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHAINPARAMSCONSTANTS_H
#define BITCOIN_CHAINPARAMSCONSTANTS_H
/**
 * Chain params constants for each tracked chain.
 */

#include <primitives/blockhash.h>
#include <uint256.h>

namespace ChainParamsConstants {
extern const BlockHash MAINNET_DEFAULT_ASSUME_VALID;
extern const uint256 MAINNET_MINIMUM_CHAIN_WORK;
extern const uint64_t MAINNET_ASSUMED_BLOCKCHAIN_SIZE;
extern const uint64_t MAINNET_ASSUMED_CHAINSTATE_SIZE;

extern const BlockHash TESTNET_DEFAULT_ASSUME_VALID;
extern const uint256 TESTNET_MINIMUM_CHAIN_WORK;
extern const uint64_t TESTNET_ASSUMED_BLOCKCHAIN_SIZE;
extern const uint64_t TESTNET_ASSUMED_CHAINSTATE_SIZE;
} // namespace ChainParamsConstants

#endif // BITCOIN_CHAINPARAMSCONSTANTS_H
