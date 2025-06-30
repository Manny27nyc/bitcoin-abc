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
// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/hkdf_sha256_32.h>

#include <cassert>
#include <cstring>

CHKDF_HMAC_SHA256_L32::CHKDF_HMAC_SHA256_L32(const uint8_t *ikm, size_t ikmlen,
                                             const std::string &salt) {
    CHMAC_SHA256((const uint8_t *)salt.data(), salt.size())
        .Write(ikm, ikmlen)
        .Finalize(m_prk);
}

void CHKDF_HMAC_SHA256_L32::Expand32(const std::string &info,
                                     uint8_t hash[OUTPUT_SIZE]) {
    // expand a 32byte key (single round)
    assert(info.size() <= 128);
    static const uint8_t one[1] = {1};
    CHMAC_SHA256(m_prk, 32)
        .Write((const uint8_t *)info.data(), info.size())
        .Write(one, 1)
        .Finalize(hash);
}
