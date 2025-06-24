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
// Copyright (c) 2018-2019 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <cashaddr.h>

#include <string>
#include <vector>

static void CashAddrEncode(benchmark::Bench &bench) {
    std::vector<uint8_t> buffer = {17,  79, 8,   99,  150, 189, 208, 162,
                                   22,  23, 203, 163, 36,  58,  147, 227,
                                   139, 2,  215, 100, 91,  38,  11,  141,
                                   253, 40, 117, 21,  16,  90,  200, 24};
    bench.batch(buffer.size()).unit("byte").run([&] {
        cashaddr::Encode("bitcoincash", buffer);
    });
}

static void CashAddrDecode(benchmark::Bench &bench) {
    const char *addrWithPrefix =
        "bitcoincash:qprnwmr02d7ky9m693qufj5mgkpf4wvssv0w86tkjd";
    const char *addrNoPrefix = "qprnwmr02d7ky9m693qufj5mgkpf4wvssv0w86tkjd";
    bench.run([&] {
        cashaddr::Decode(addrWithPrefix, "bitcoincash");
        cashaddr::Decode(addrNoPrefix, "bitcoincash");
    });
}

BENCHMARK(CashAddrEncode);
BENCHMARK(CashAddrDecode);
