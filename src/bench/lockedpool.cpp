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

#include <support/lockedpool.h>

#include <vector>

#define ASIZE 2048
#define MSIZE 2048

static void BenchLockedPool(benchmark::Bench &bench) {
    void *synth_base = reinterpret_cast<void *>(0x08000000);
    const size_t synth_size = 1024 * 1024;
    Arena b(synth_base, synth_size, 16);

    std::vector<void *> addr;
    for (int x = 0; x < ASIZE; ++x) {
        addr.push_back(nullptr);
    }
    uint32_t s = 0x12345678;
    bench.run([&] {
        int idx = s & (addr.size() - 1);
        if (s & 0x80000000) {
            b.free(addr[idx]);
            addr[idx] = nullptr;
        } else if (!addr[idx]) {
            addr[idx] = b.alloc((s >> 16) & (MSIZE - 1));
        }
        bool lsb = s & 1;
        s >>= 1;
        if (lsb) {
            // LFSR period 0xf7ffffe0
            s ^= 0xf00f00f0;
        }
    });
    for (void *ptr : addr) {
        b.free(ptr);
    }
    addr.clear();
}

BENCHMARK(BenchLockedPool);
