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
// Copyright (c) 2017-2019 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/test/guiutiltests.h>

#include <chainparams.h>
#include <config.h>
#include <key_io.h>
#include <qt/guiutil.h>

namespace {

class GUIUtilTestConfig : public DummyConfig {
public:
    GUIUtilTestConfig()
        : DummyConfig(CBaseChainParams::MAIN), useCashAddr(true) {}
    void SetCashAddrEncoding(bool b) override { useCashAddr = b; }
    bool UseCashAddrEncoding() const override { return useCashAddr; }

private:
    bool useCashAddr;
};

} // namespace

void GUIUtilTests::dummyAddressTest() {
    GUIUtilTestConfig config;
    const CChainParams &params = config.GetChainParams();

    std::string dummyaddr;

    dummyaddr = GUIUtil::DummyAddress(params);
    QVERIFY(!IsValidDestinationString(dummyaddr, params));
    QVERIFY(!dummyaddr.empty());
}

void GUIUtilTests::toCurrentEncodingTest() {
    GUIUtilTestConfig config;
    const CChainParams &params = config.GetChainParams();

    // garbage in, garbage out
    QVERIFY(GUIUtil::convertToCashAddr(params, "garbage") == "garbage");

    QString cashaddr_pubkey =
        "ecash:qpm2qsznhks23z7629mms6s4cwef74vcwva87rkuu2";

    QString base58_pubkey = "1BpEi6DfDAUFd7GtittLSdBeYJvcoaVggu";

    QVERIFY(GUIUtil::convertToCashAddr(params, cashaddr_pubkey) ==
            cashaddr_pubkey);
    QVERIFY(GUIUtil::convertToCashAddr(params, base58_pubkey) ==
            cashaddr_pubkey);
}
