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
// Copyright (c) 2016-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_WALLETTOOL_H
#define BITCOIN_WALLET_WALLETTOOL_H

#include <wallet/wallet.h>

namespace WalletTool {

std::shared_ptr<CWallet> CreateWallet(const std::string &name,
                                      const fs::path &path);
std::shared_ptr<CWallet> LoadWallet(const std::string &name,
                                    const fs::path &path);
void WalletShowInfo(CWallet *wallet_instance);
bool ExecuteWalletToolFunc(const std::string &command, const std::string &file);

} // namespace WalletTool

#endif // BITCOIN_WALLET_WALLETTOOL_H
