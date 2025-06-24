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
// Copyright (c) 2015-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

// Extremely fast-running benchmark:
#include <cmath>

volatile double sum = 0.0; // volatile, global so not optimized away

static void Trig(benchmark::Bench &bench) {
    double d = 0.01;
    bench.run([&] {
        sum += sin(d);
        d += 0.000001;
    });
}

BENCHMARK(Trig);
