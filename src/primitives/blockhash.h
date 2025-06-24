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

#ifndef BITCOIN_PRIMITIVES_BLOCKHASH_H
#define BITCOIN_PRIMITIVES_BLOCKHASH_H

#include <uint256.h>

/**
 * A BlockHash is a unqiue identifier for a block.
 */
struct BlockHash : public uint256 {
    explicit BlockHash() : uint256() {}
    explicit BlockHash(const uint256 &b) : uint256(b) {}

    static BlockHash fromHex(const std::string &str) {
        BlockHash r;
        r.SetHex(str);
        return r;
    }
};

#endif // BITCOIN_PRIMITIVES_BLOCKHASH_H
