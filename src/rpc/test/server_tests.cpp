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
// Copyright (c) 2019 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>
#include <test/util/setup_common.h>
#include <util/system.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(server_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(server_IsDeprecatedRPCEnabled) {
    ArgsManager testArgs;
    testArgs.AddArg("-deprecatedrpc", "", ArgsManager::ALLOW_ANY,
                    OptionsCategory::OPTIONS);

    const char *argv_test[] = {"bitcoind", "-deprecatedrpc=foo",
                               "-deprecatedrpc=bar"};

    std::string error;
    BOOST_CHECK_MESSAGE(testArgs.ParseParameters(3, (char **)argv_test, error),
                        error);

    BOOST_CHECK(IsDeprecatedRPCEnabled(testArgs, "foo") == true);
    BOOST_CHECK(IsDeprecatedRPCEnabled(testArgs, "bar") == true);
    BOOST_CHECK(IsDeprecatedRPCEnabled(testArgs, "bob") == false);
}

BOOST_AUTO_TEST_SUITE_END()
