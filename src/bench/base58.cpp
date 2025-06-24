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

#include <base58.h>

#include <array>
#include <vector>

static void Base58Encode(benchmark::Bench &bench) {
    static const std::vector<uint8_t> buffer = {
        17,  79,  8,   99,  150, 189, 208, 162, 22,  23, 203,
        163, 36,  58,  147, 227, 139, 2,   215, 100, 91, 38,
        11,  141, 253, 40,  117, 21,  16,  90,  200, 24};
    bench.batch(buffer.size()).unit("byte").run([&] { EncodeBase58(buffer); });
}

static void Base58CheckEncode(benchmark::Bench &bench) {
    static const std::vector<uint8_t> buffer = {
        17,  79,  8,   99,  150, 189, 208, 162, 22,  23, 203,
        163, 36,  58,  147, 227, 139, 2,   215, 100, 91, 38,
        11,  141, 253, 40,  117, 21,  16,  90,  200, 24};
    bench.batch(buffer.size()).unit("byte").run([&] {
        EncodeBase58Check(buffer);
    });
}

static void Base58Decode(benchmark::Bench &bench) {
    const char *addr = "17VZNX1SN5NtKa8UQFxwQbFeFc3iqRYhem";
    std::vector<uint8_t> vch;
    bench.batch(strlen(addr)).unit("byte").run([&] {
        (void)DecodeBase58(addr, vch, 64);
    });
}

BENCHMARK(Base58Encode);
BENCHMARK(Base58CheckEncode);
BENCHMARK(Base58Decode);
