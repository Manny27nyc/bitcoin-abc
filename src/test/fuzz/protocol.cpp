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

#include <protocol.h>

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

void test_one_input(const std::vector<uint8_t> &buffer) {
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::optional<CInv> inv =
        ConsumeDeserializable<CInv>(fuzzed_data_provider);
    if (!inv) {
        return;
    }
    try {
        (void)inv->GetCommand();
    } catch (const std::out_of_range &) {
    }
    (void)inv->GetKind();
    (void)inv->IsGenBlkMsg();
    (void)inv->IsMsgTx();
    (void)inv->ToString();
    const std::optional<CInv> another_inv =
        ConsumeDeserializable<CInv>(fuzzed_data_provider);
    if (!another_inv) {
        return;
    }
    (void)(*inv < *another_inv);
}
