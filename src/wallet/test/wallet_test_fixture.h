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
// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_TEST_WALLET_TEST_FIXTURE_H
#define BITCOIN_WALLET_TEST_WALLET_TEST_FIXTURE_H

#include <interfaces/chain.h>
#include <interfaces/wallet.h>
#include <node/context.h>
#include <util/check.h>
#include <wallet/wallet.h>

#include <test/util/setup_common.h>

#include <memory>

/**
 * Testing setup and teardown for wallet.
 */
struct WalletTestingSetup : public TestingSetup {
    explicit WalletTestingSetup(
        const std::string &chainName = CBaseChainParams::MAIN);

    std::unique_ptr<interfaces::Chain> m_chain;
    std::unique_ptr<interfaces::ChainClient> m_chain_client =
        interfaces::MakeWalletClient(*m_chain, *Assert(m_node.args), {});
    CWallet m_wallet;
    std::unique_ptr<interfaces::Handler> m_chain_notifications_handler;
};

#endif // BITCOIN_WALLET_TEST_WALLET_TEST_FIXTURE_H
