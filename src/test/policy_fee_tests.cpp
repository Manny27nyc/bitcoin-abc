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
// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <amount.h>
#include <feerate.h>
#include <policy/fees.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(policy_fee_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(FeeRounder) {
    FeeFilterRounder fee_rounder{CFeeRate{1000 * SATOSHI}};

    // check that 1000 rounds to 974 or 1071
    std::set<Amount> results;
    while (results.size() < 2) {
        results.emplace(fee_rounder.round(1000 * SATOSHI));
    }
    BOOST_CHECK_EQUAL(*results.begin(), 974 * SATOSHI);
    BOOST_CHECK_EQUAL(*++results.begin(), 1071 * SATOSHI);

    // check that negative amounts rounds to 0
    BOOST_CHECK_EQUAL(fee_rounder.round(-0 * SATOSHI), Amount::zero());
    BOOST_CHECK_EQUAL(fee_rounder.round(-1 * SATOSHI), Amount::zero());

    // check that MAX_MONEY rounds to 9170997
    BOOST_CHECK_EQUAL(fee_rounder.round(MAX_MONEY), 9170997 * SATOSHI);
}

BOOST_AUTO_TEST_SUITE_END()
