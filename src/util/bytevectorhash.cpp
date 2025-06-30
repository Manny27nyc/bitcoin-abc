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

#include <crypto/siphash.h>
#include <random.h>
#include <util/bytevectorhash.h>

ByteVectorHash::ByteVectorHash() {
    GetRandBytes(reinterpret_cast<uint8_t *>(&m_k0), sizeof(m_k0));
    GetRandBytes(reinterpret_cast<uint8_t *>(&m_k1), sizeof(m_k1));
}

size_t ByteVectorHash::operator()(const std::vector<uint8_t> &input) const {
    return CSipHasher(m_k0, m_k1).Write(input.data(), input.size()).Finalize();
}
