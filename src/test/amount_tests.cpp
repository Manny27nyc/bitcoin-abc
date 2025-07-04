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
// Copyright (c) 2016-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <amount.h>

#include <network.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <array>

BOOST_FIXTURE_TEST_SUITE(amount_tests, BasicTestingSetup)

static void CheckAmounts(int64_t aval, int64_t bval) {
    Amount a(aval * SATOSHI), b(bval * SATOSHI);

    // Equality
    BOOST_CHECK_EQUAL(a == b, aval == bval);
    BOOST_CHECK_EQUAL(b == a, aval == bval);

    BOOST_CHECK_EQUAL(a != b, aval != bval);
    BOOST_CHECK_EQUAL(b != a, aval != bval);

    // Comparison
    BOOST_CHECK_EQUAL(a < b, aval < bval);
    BOOST_CHECK_EQUAL(b < a, bval < aval);

    BOOST_CHECK_EQUAL(a > b, aval > bval);
    BOOST_CHECK_EQUAL(b > a, bval > aval);

    BOOST_CHECK_EQUAL(a <= b, aval <= bval);
    BOOST_CHECK_EQUAL(b <= a, bval <= aval);

    BOOST_CHECK_EQUAL(a >= b, aval >= bval);
    BOOST_CHECK_EQUAL(b >= a, bval >= aval);

    // Unary minus
    BOOST_CHECK_EQUAL(-a, -aval * SATOSHI);
    BOOST_CHECK_EQUAL(-b, -bval * SATOSHI);

    // Addition and subtraction.
    BOOST_CHECK_EQUAL(a + b, b + a);
    BOOST_CHECK_EQUAL(a + b, (aval + bval) * SATOSHI);

    BOOST_CHECK_EQUAL(a - b, -(b - a));
    BOOST_CHECK_EQUAL(a - b, (aval - bval) * SATOSHI);

    // Multiplication
    BOOST_CHECK_EQUAL(aval * b, bval * a);
    BOOST_CHECK_EQUAL(aval * b, (aval * bval) * SATOSHI);

    // Division
    if (b != Amount::zero()) {
        BOOST_CHECK_EQUAL(a / b, aval / bval);
        BOOST_CHECK_EQUAL(a / bval, (a / b) * SATOSHI);
    }

    if (a != Amount::zero()) {
        BOOST_CHECK_EQUAL(b / a, bval / aval);
        BOOST_CHECK_EQUAL(b / aval, (b / a) * SATOSHI);
    }

    // Modulus
    if (b != Amount::zero()) {
        BOOST_CHECK_EQUAL(a % b, a % bval);
        BOOST_CHECK_EQUAL(a % b, (aval % bval) * SATOSHI);
    }

    if (a != Amount::zero()) {
        BOOST_CHECK_EQUAL(b % a, b % aval);
        BOOST_CHECK_EQUAL(b % a, (bval % aval) * SATOSHI);
    }

    // OpAssign
    Amount v;
    BOOST_CHECK_EQUAL(v, Amount::zero());
    v += a;
    BOOST_CHECK_EQUAL(v, a);
    v += b;
    BOOST_CHECK_EQUAL(v, a + b);
    v += b;
    BOOST_CHECK_EQUAL(v, a + 2 * b);
    v -= 2 * a;
    BOOST_CHECK_EQUAL(v, 2 * b - a);
}

BOOST_AUTO_TEST_CASE(AmountTests) {
    std::array<int64_t, 8> values = {{-23, -1, 0, 1, 2, 3, 42, 99999999}};

    for (int64_t i : values) {
        for (int64_t j : values) {
            CheckAmounts(i, j);
        }
    }

    BOOST_CHECK_EQUAL(COIN + COIN, 2 * COIN);
    BOOST_CHECK_EQUAL(2 * COIN + COIN, 3 * COIN);
    BOOST_CHECK_EQUAL(-1 * COIN + COIN, Amount::zero());

    BOOST_CHECK_EQUAL(COIN - COIN, Amount::zero());
    BOOST_CHECK_EQUAL(COIN - 2 * COIN, -1 * COIN);
}

BOOST_AUTO_TEST_CASE(MoneyRangeTest) {
    BOOST_CHECK_EQUAL(MoneyRange(-SATOSHI), false);
    BOOST_CHECK_EQUAL(MoneyRange(Amount::zero()), true);
    BOOST_CHECK_EQUAL(MoneyRange(SATOSHI), true);
    BOOST_CHECK_EQUAL(MoneyRange(MAX_MONEY), true);
    BOOST_CHECK_EQUAL(MoneyRange(MAX_MONEY + SATOSHI), false);
}

BOOST_AUTO_TEST_CASE(BinaryOperatorTest) {
    CFeeRate a, b;
    a = CFeeRate(1 * SATOSHI);
    b = CFeeRate(2 * SATOSHI);
    BOOST_CHECK(a < b);
    BOOST_CHECK(b > a);
    BOOST_CHECK(a == a);
    BOOST_CHECK(a <= b);
    BOOST_CHECK(a <= a);
    BOOST_CHECK(b >= a);
    BOOST_CHECK(b >= b);
    // a should be 0.00000002 BTC/kB now
    a += a;
    BOOST_CHECK(a == b);
}

BOOST_AUTO_TEST_CASE(ToStringTest) {
    CFeeRate feeRate;
    feeRate = CFeeRate(1 * SATOSHI);
    BOOST_CHECK_EQUAL(feeRate.ToString(),
                      "0.01 " + Currency::get().ticker + "/kB");
}

BOOST_AUTO_TEST_SUITE_END()
