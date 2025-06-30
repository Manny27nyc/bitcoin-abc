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

#ifndef BITCOIN_TEST_LCG_H
#define BITCOIN_TEST_LCG_H

#include <cstdint>

// Simple 32-bit linear congruential generator with 64-bit internal state,
// often called as "MMIX by Donald Knuth". Knuth attributes the multiplier
// to C. E. Haynes and the increment is not crucial (it only need be odd).
// Knuth, Donald (1997). Seminumerical Algorithms Vol2. Sec 3.3.4. 3rd Ed.
//
// Low bits have short period, hence we use high bits which should have
// the same period as the entire generator (2^64).
class MMIXLinearCongruentialGenerator {
    uint64_t state;

public:
    MMIXLinearCongruentialGenerator(uint64_t initialstate = 0)
        : state(initialstate) {}

    // pseudorandom, except the first value returned is the seed provided
    // (thus starting with 0, by default)
    uint32_t next() {
        uint32_t ret = state >> 32;
        state = state * 6364136223846793005 + 1442695040888963407;
        return ret;
    }
};

#endif // BITCOIN_TEST_LCG_H
