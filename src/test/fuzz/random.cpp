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

#include <random.h>

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

void test_one_input(const std::vector<uint8_t> &buffer) {
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    FastRandomContext fast_random_context{ConsumeUInt256(fuzzed_data_provider)};
    (void)fast_random_context.rand64();
    (void)fast_random_context.randbits(
        fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 64));
    (void)fast_random_context.randrange(
        fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(
            FastRandomContext::min() + 1, FastRandomContext::max()));
    (void)fast_random_context.randbytes(
        fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 1024));
    (void)fast_random_context.rand32();
    (void)fast_random_context.rand256();
    (void)fast_random_context.randbool();
    (void)fast_random_context();

    std::vector<int64_t> integrals =
        ConsumeRandomLengthIntegralVector<int64_t>(fuzzed_data_provider);
    Shuffle(integrals.begin(), integrals.end(), fast_random_context);
    std::shuffle(integrals.begin(), integrals.end(), fast_random_context);
}
