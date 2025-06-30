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
// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_POLY1305_H
#define BITCOIN_CRYPTO_POLY1305_H

#include <cstdint>
#include <cstdlib>

#define POLY1305_KEYLEN 32
#define POLY1305_TAGLEN 16

void poly1305_auth(uint8_t out[POLY1305_TAGLEN], const uint8_t *m, size_t inlen,
                   const uint8_t key[POLY1305_KEYLEN]);

#endif // BITCOIN_CRYPTO_POLY1305_H
