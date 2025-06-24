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

#include <addrdb.h>

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

void test_one_input(const std::vector<uint8_t> &buffer) {
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    [&] {
        switch (fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 2)) {
            case 0:
                return CBanEntry{
                    fuzzed_data_provider.ConsumeIntegral<int64_t>()};
                break;
            case 1: {
                const std::optional<CBanEntry> ban_entry =
                    ConsumeDeserializable<CBanEntry>(fuzzed_data_provider);
                if (ban_entry) {
                    return *ban_entry;
                }
                break;
            }
        }
        return CBanEntry{};
    }();
}
