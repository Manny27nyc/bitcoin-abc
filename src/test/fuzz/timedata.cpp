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

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <timedata.h>

#include <cstdint>
#include <string>
#include <vector>

void test_one_input(const std::vector<uint8_t> &buffer) {
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const unsigned int max_size =
        fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, 1000);
    // Divide by 2 to avoid signed integer overflow in .median()
    const int64_t initial_value =
        fuzzed_data_provider.ConsumeIntegral<int64_t>() / 2;
    CMedianFilter<int64_t> median_filter{max_size, initial_value};
    while (fuzzed_data_provider.remaining_bytes() > 0) {
        (void)median_filter.median();
        assert(median_filter.size() > 0);
        assert(static_cast<size_t>(median_filter.size()) ==
               median_filter.sorted().size());
        assert(static_cast<unsigned int>(median_filter.size()) <= max_size ||
               max_size == 0);
        // Divide by 2 to avoid signed integer overflow in .median()
        median_filter.input(fuzzed_data_provider.ConsumeIntegral<int64_t>() /
                            2);
    }
}
