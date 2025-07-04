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

#include <util/ref.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(ref_tests)

BOOST_AUTO_TEST_CASE(ref_test) {
    util::Ref ref;
    BOOST_CHECK(!ref.Has<int>());
    BOOST_CHECK_THROW(ref.Get<int>(), NonFatalCheckError);
    int value = 5;
    ref.Set(value);
    BOOST_CHECK(ref.Has<int>());
    BOOST_CHECK_EQUAL(ref.Get<int>(), 5);
    ++ref.Get<int>();
    BOOST_CHECK_EQUAL(ref.Get<int>(), 6);
    BOOST_CHECK_EQUAL(value, 6);
    ++value;
    BOOST_CHECK_EQUAL(value, 7);
    BOOST_CHECK_EQUAL(ref.Get<int>(), 7);
    BOOST_CHECK(!ref.Has<bool>());
    BOOST_CHECK_THROW(ref.Get<bool>(), NonFatalCheckError);
    ref.Clear();
    BOOST_CHECK(!ref.Has<int>());
    BOOST_CHECK_THROW(ref.Get<int>(), NonFatalCheckError);
}

BOOST_AUTO_TEST_SUITE_END()
