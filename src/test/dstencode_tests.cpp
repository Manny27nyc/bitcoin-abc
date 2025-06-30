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

#include <key_io.h>

#include <chainparams.h>
#include <config.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

namespace {

class DummyCashAddrConfig : public DummyConfig {
public:
    DummyCashAddrConfig()
        : DummyConfig(CBaseChainParams::MAIN), useCashAddr(false) {}
    void SetCashAddrEncoding(bool b) override { useCashAddr = b; }
    bool UseCashAddrEncoding() const override { return useCashAddr; }

private:
    bool useCashAddr;
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(dstencode_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(test_addresses) {
    std::vector<uint8_t> hash = {118, 160, 64,  83,  189, 160, 168,
                                 139, 218, 81,  119, 184, 106, 21,
                                 195, 178, 159, 85,  152, 115};

    const CTxDestination dstKey = PKHash(uint160(hash));
    const CTxDestination dstScript = ScriptHash(uint160(hash));

    std::string cashaddr_pubkey =
        "ecash:qpm2qsznhks23z7629mms6s4cwef74vcwva87rkuu2";
    std::string cashaddr_script =
        "ecash:ppm2qsznhks23z7629mms6s4cwef74vcwv2zrv3l8h";
    std::string base58_pubkey = "1BpEi6DfDAUFd7GtittLSdBeYJvcoaVggu";
    std::string base58_script = "3CWFddi6m4ndiGyKqzYvsFYagqDLPVMTzC";

    DummyCashAddrConfig config;

    // Check encoding
    config.SetCashAddrEncoding(true);
    BOOST_CHECK_EQUAL(cashaddr_pubkey, EncodeDestination(dstKey, config));
    BOOST_CHECK_EQUAL(cashaddr_script, EncodeDestination(dstScript, config));
    config.SetCashAddrEncoding(false);
    BOOST_CHECK_EQUAL(base58_pubkey, EncodeDestination(dstKey, config));
    BOOST_CHECK_EQUAL(base58_script, EncodeDestination(dstScript, config));

    // Check decoding
    const CChainParams &params = config.GetChainParams();
    BOOST_CHECK(dstKey == DecodeDestination(cashaddr_pubkey, params));
    BOOST_CHECK(dstScript == DecodeDestination(cashaddr_script, params));
    BOOST_CHECK(dstKey == DecodeDestination(base58_pubkey, params));
    BOOST_CHECK(dstScript == DecodeDestination(base58_script, params));

    // Validation
    BOOST_CHECK(IsValidDestinationString(cashaddr_pubkey, params));
    BOOST_CHECK(IsValidDestinationString(cashaddr_script, params));
    BOOST_CHECK(IsValidDestinationString(base58_pubkey, params));
    BOOST_CHECK(IsValidDestinationString(base58_script, params));
    BOOST_CHECK(!IsValidDestinationString("notvalid", params));
}

BOOST_AUTO_TEST_SUITE_END()
