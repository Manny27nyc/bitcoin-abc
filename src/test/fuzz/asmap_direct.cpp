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

#include <util/asmap.h>

#include <test/fuzz/fuzz.h>

#include <cassert>
#include <cstdint>
#include <optional>
#include <vector>

void test_one_input(const std::vector<uint8_t> &buffer) {
    // Encoding: [asmap using 1 bit / byte] 0xFF [addr using 1 bit / byte]
    std::optional<size_t> sep_pos_opt;
    for (size_t pos = 0; pos < buffer.size(); ++pos) {
        uint8_t x = buffer[pos];
        if ((x & 0xFE) == 0) {
            continue;
        }
        if (x == 0xFF) {
            if (sep_pos_opt) {
                return;
            }
            sep_pos_opt = pos;
        } else {
            return;
        }
    }
    if (!sep_pos_opt) {
        // Needs exactly 1 separator
        return;
    }
    const size_t sep_pos{sep_pos_opt.value()};
    if (buffer.size() - sep_pos - 1 > 128) {
        // At most 128 bits in IP address
        return;
    }

    // Checks on asmap
    std::vector<bool> asmap(buffer.begin(), buffer.begin() + sep_pos);
    if (SanityCheckASMap(asmap, buffer.size() - 1 - sep_pos)) {
        // Verify that for valid asmaps, no prefix (except up to 7 zero padding
        // bits) is valid.
        std::vector<bool> asmap_prefix = asmap;
        while (!asmap_prefix.empty() &&
               asmap_prefix.size() + 7 > asmap.size() &&
               asmap_prefix.back() == false) {
            asmap_prefix.pop_back();
        }
        while (!asmap_prefix.empty()) {
            asmap_prefix.pop_back();
            assert(
                !SanityCheckASMap(asmap_prefix, buffer.size() - 1 - sep_pos));
        }
        // No address input should trigger assertions in interpreter
        std::vector<bool> addr(buffer.begin() + sep_pos + 1, buffer.end());
        (void)Interpret(asmap, addr);
    }
}
