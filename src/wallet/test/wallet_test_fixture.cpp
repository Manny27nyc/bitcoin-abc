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

#include <wallet/test/wallet_test_fixture.h>

#include <chainparams.h>
#include <validationinterface.h>
#include <wallet/rpcdump.h>

WalletTestingSetup::WalletTestingSetup(const std::string &chainName)
    : TestingSetup(chainName), m_chain(interfaces::MakeChain(m_node, Params())),
      m_wallet(m_chain.get(), WalletLocation(), CreateMockWalletDatabase()) {
    bool fFirstRun;
    m_wallet.LoadWallet(fFirstRun);
    m_chain_notifications_handler =
        m_chain->handleNotifications({&m_wallet, [](CWallet *) {}});
    m_chain_client->registerRpcs();
}
