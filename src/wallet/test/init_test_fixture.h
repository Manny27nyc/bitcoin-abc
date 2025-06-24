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
// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_TEST_INIT_TEST_FIXTURE_H
#define BITCOIN_WALLET_TEST_INIT_TEST_FIXTURE_H

#include <interfaces/chain.h>

#include <node/context.h>
#include <test/util/setup_common.h>

struct InitWalletDirTestingSetup : public BasicTestingSetup {
    explicit InitWalletDirTestingSetup(
        const std::string &chainName = CBaseChainParams::MAIN);
    ~InitWalletDirTestingSetup();
    void SetWalletDir(const fs::path &walletdir_path);

    fs::path m_datadir;
    fs::path m_cwd;
    std::map<std::string, fs::path> m_walletdir_path_cases;
    std::unique_ptr<interfaces::Chain> m_chain;
    std::unique_ptr<interfaces::ChainClient> m_chain_client;
};

#endif // BITCOIN_WALLET_TEST_INIT_TEST_FIXTURE_H
