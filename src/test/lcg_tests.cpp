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
// Copyright (c) 2019 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/lcg.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(lcg_tests)

BOOST_AUTO_TEST_CASE(lcg_testvalues) {
    {
        MMIXLinearCongruentialGenerator lcg;
        // We want that the first iteration is 0 which is a helpful special
        // case.
        BOOST_CHECK_EQUAL(lcg.next(), 0x00000000);
        for (int i = 0; i < 99; i++) {
            lcg.next();
        }
        // Make sure the LCG is producing expected value after many iterations.
        // This ensures mul and add overflows are acting as expected on this
        // architecture.
        BOOST_CHECK_EQUAL(lcg.next(), 0xf306b780);
    }
    {
        MMIXLinearCongruentialGenerator lcg(42);
        // This also should make first iteration as 0.
        BOOST_CHECK_EQUAL(lcg.next(), 0x00000000);
        for (int i = 0; i < 99; i++) {
            lcg.next();
        }
        // Make sure the LCG is producing expected value after many iterations.
        // This ensures mul and add overflows are acting as expected on this
        // architecture.
        BOOST_CHECK_EQUAL(lcg.next(), 0x3b96faf3);
    }
    {
        // just some big seed
        MMIXLinearCongruentialGenerator lcg(0xdeadbeef00000000);
        BOOST_CHECK_EQUAL(lcg.next(), 0xdeadbeef);
        for (int i = 0; i < 99; i++) {
            lcg.next();
        }
        // Make sure the LCG is producing expected value after many iterations.
        // This ensures mul and add overflows are acting as expected on this
        // architecture.
        BOOST_CHECK_EQUAL(lcg.next(), 0x6b00b1df);
    }
}

BOOST_AUTO_TEST_SUITE_END()
