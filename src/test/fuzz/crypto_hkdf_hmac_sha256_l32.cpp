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

#include <crypto/hkdf_sha256_32.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <string>
#include <vector>

void test_one_input(const std::vector<uint8_t> &buffer) {
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    const std::vector<uint8_t> initial_key_material =
        ConsumeRandomLengthByteVector(fuzzed_data_provider);

    CHKDF_HMAC_SHA256_L32 hkdf_hmac_sha256_l32(
        initial_key_material.data(), initial_key_material.size(),
        fuzzed_data_provider.ConsumeRandomLengthString(1024));
    while (fuzzed_data_provider.ConsumeBool()) {
        std::vector<uint8_t> out(32);
        hkdf_hmac_sha256_l32.Expand32(
            fuzzed_data_provider.ConsumeRandomLengthString(128), out.data());
    }
}
