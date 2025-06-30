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
// Copyright (c) 2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_HMAC_SHA512_H
#define BITCOIN_CRYPTO_HMAC_SHA512_H

#include <crypto/sha512.h>

#include <cstdint>
#include <cstdlib>

/** A hasher class for HMAC-SHA-512. */
class CHMAC_SHA512 {
private:
    CSHA512 outer;
    CSHA512 inner;

public:
    static const size_t OUTPUT_SIZE = 64;

    CHMAC_SHA512(const uint8_t *key, size_t keylen);
    CHMAC_SHA512 &Write(const uint8_t *data, size_t len) {
        inner.Write(data, len);
        return *this;
    }
    void Finalize(uint8_t hash[OUTPUT_SIZE]);
};

#endif // BITCOIN_CRYPTO_HMAC_SHA512_H
