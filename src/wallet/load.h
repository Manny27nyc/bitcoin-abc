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
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_LOAD_H
#define BITCOIN_WALLET_LOAD_H

#include <string>
#include <vector>

class ArgsManager;
class CChainParams;
class CScheduler;

namespace interfaces {
class Chain;
} // namespace interfaces

//! Responsible for reading and validating the -wallet arguments and verifying
//! the wallet database.
bool VerifyWallets(const CChainParams &chainParams, interfaces::Chain &chain,
                   const std::vector<std::string> &wallet_files);

//! Load wallet databases.
bool LoadWallets(const CChainParams &chainParams, interfaces::Chain &chain,
                 const std::vector<std::string> &wallet_files);

//! Complete startup of wallets.
void StartWallets(CScheduler &scheduler, const ArgsManager &args);

//! Flush all wallets in preparation for shutdown.
void FlushWallets();

//! Stop all wallets. Wallets will be flushed first.
void StopWallets();

//! Close all wallets.
void UnloadWallets();

#endif // BITCOIN_WALLET_LOAD_H
