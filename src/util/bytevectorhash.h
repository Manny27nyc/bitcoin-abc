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
// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_BYTEVECTORHASH_H
#define BITCOIN_UTIL_BYTEVECTORHASH_H

#include <cstdint>
#include <vector>

/**
 * Implementation of Hash named requirement for types that internally store a
 * byte array. This may be used as the hash function in std::unordered_set or
 * std::unordered_map over such types. Internally, this uses a random instance
 * of SipHash-2-4.
 */
class ByteVectorHash final {
private:
    uint64_t m_k0, m_k1;

public:
    ByteVectorHash();
    size_t operator()(const std::vector<uint8_t> &input) const;
};

#endif // BITCOIN_UTIL_BYTEVECTORHASH_H
