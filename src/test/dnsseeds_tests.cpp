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
// Copyright (c) 2020-2021 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <dnsseeds.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(dnsseeds_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(override_dns_seed) {
    // No override should always provide some DNS seeds
    const auto params = CreateChainParams(CBaseChainParams::MAIN);
    BOOST_CHECK(GetRandomizedDNSSeeds(*params).size() > 0);

    // Overriding should only return that DNS seed
    gArgs.ForceSetArg("-overridednsseed", "localhost");
    BOOST_CHECK(GetRandomizedDNSSeeds(*params) ==
                std::vector<std::string>{{"localhost"}});
}

BOOST_AUTO_TEST_SUITE_END()
