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

#include <wallet/test/init_test_fixture.h>

#include <chainparams.h>
#include <fs.h>
#include <util/check.h>
#include <util/system.h>

InitWalletDirTestingSetup::InitWalletDirTestingSetup(
    const std::string &chainName)
    : BasicTestingSetup(chainName) {
    m_chain = interfaces::MakeChain(m_node, Params());
    m_chain_client = MakeWalletClient(*m_chain, *Assert(m_node.args), {});

    std::string sep;
    sep += fs::path::preferred_separator;

    m_datadir = GetDataDir();
    m_cwd = fs::current_path();

    m_walletdir_path_cases["default"] = m_datadir / "wallets";
    m_walletdir_path_cases["custom"] = m_datadir / "my_wallets";
    m_walletdir_path_cases["nonexistent"] = m_datadir / "path_does_not_exist";
    m_walletdir_path_cases["file"] = m_datadir / "not_a_directory.dat";
    m_walletdir_path_cases["trailing"] = m_datadir / "wallets" / sep;
    m_walletdir_path_cases["trailing2"] = m_datadir / "wallets" / sep / sep;

    fs::current_path(m_datadir);
    m_walletdir_path_cases["relative"] = "wallets";

    fs::create_directories(m_walletdir_path_cases["default"]);
    fs::create_directories(m_walletdir_path_cases["custom"]);
    fs::create_directories(m_walletdir_path_cases["relative"]);
    std::ofstream f(m_walletdir_path_cases["file"].BOOST_FILESYSTEM_C_STR);
    f.close();
}

InitWalletDirTestingSetup::~InitWalletDirTestingSetup() {
    fs::current_path(m_cwd);
}

void InitWalletDirTestingSetup::SetWalletDir(const fs::path &walletdir_path) {
    gArgs.ForceSetArg("-walletdir", walletdir_path.string());
}
