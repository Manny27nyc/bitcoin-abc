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
// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bloom.h>

static void RollingBloom(benchmark::Bench &bench) {
    CRollingBloomFilter filter(120000, 0.000001);
    std::vector<uint8_t> data(32);
    uint32_t count = 0;
    bench.run([&] {
        count++;
        data[0] = count;
        data[1] = count >> 8;
        data[2] = count >> 16;
        data[3] = count >> 24;
        filter.insert(data);

        data[0] = count >> 24;
        data[1] = count >> 16;
        data[2] = count >> 8;
        data[3] = count;
        filter.contains(data);
    });
}

static void RollingBloomReset(benchmark::Bench &bench) {
    CRollingBloomFilter filter(120000, 0.000001);
    bench.run([&] { filter.reset(); });
}

BENCHMARK(RollingBloom);
BENCHMARK(RollingBloomReset);
