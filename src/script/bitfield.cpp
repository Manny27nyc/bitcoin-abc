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
// Copyright (c) 2019 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/bitfield.h>

#include <script/script_error.h>

#include <cstddef>
#include <limits>

bool DecodeBitfield(const std::vector<uint8_t> &vch, unsigned size,
                    uint32_t &bitfield, ScriptError *serror) {
    if (size > 32) {
        return set_error(serror, ScriptError::INVALID_BITFIELD_SIZE);
    }

    const size_t bitfield_size = (size + 7) / 8;
    if (vch.size() != bitfield_size) {
        return set_error(serror, ScriptError::INVALID_BITFIELD_SIZE);
    }

    bitfield = 0;
    for (size_t i = 0; i < bitfield_size; i++) {
        // Decode the bitfield as little endian.
        bitfield |= uint32_t(vch[i]) << (8 * i);
    }

    const uint32_t mask = (uint64_t(1) << size) - 1;
    if ((bitfield & mask) != bitfield) {
        return set_error(serror, ScriptError::INVALID_BIT_RANGE);
    }

    return true;
}
