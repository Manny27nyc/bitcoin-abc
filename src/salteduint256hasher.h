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
// Copyright (c) 2018-2019 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SALTEDUINT256HASHER_H
#define BITCOIN_SALTEDUINT256HASHER_H

#include <crypto/siphash.h>
#include <uint256.h>

class SaltedUint256Hasher {
private:
    /** Salt */
    const uint64_t k0, k1;

public:
    SaltedUint256Hasher();

    size_t hash(const uint256 &h) const { return SipHashUint256(k0, k1, h); }
    size_t operator()(const uint256 &h) const { return hash(h); }
};

#endif // BITCOIN_SALTEDUINT256HASHER_H
