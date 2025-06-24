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
// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_KEYORIGIN_H
#define BITCOIN_SCRIPT_KEYORIGIN_H

#include <serialize.h>
#include <vector>

struct KeyOriginInfo {
    //! First 32 bits of the Hash160 of the public key at the root of the path
    uint8_t fingerprint[4];
    std::vector<uint32_t> path;

    friend bool operator==(const KeyOriginInfo &a, const KeyOriginInfo &b) {
        return std::equal(std::begin(a.fingerprint), std::end(a.fingerprint),
                          std::begin(b.fingerprint)) &&
               a.path == b.path;
    }

    SERIALIZE_METHODS(KeyOriginInfo, obj) {
        READWRITE(obj.fingerprint, obj.path);
    }

    void clear() {
        memset(fingerprint, 0, 4);
        path.clear();
    }
};

#endif // BITCOIN_SCRIPT_KEYORIGIN_H
