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
// Copyright (c) 2020 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_AVALANCHE_TEST_UTIL_H
#define BITCOIN_AVALANCHE_TEST_UTIL_H

#include <avalanche/proof.h>
#include <pubkey.h>

#include <cstdio>

namespace avalanche {

constexpr uint32_t MIN_VALID_PROOF_SCORE = 100 * PROOF_DUST_THRESHOLD / COIN;

Proof buildRandomProof(uint32_t score, const CPubKey &master = CPubKey());

bool hasDustStake(const Proof &proof);

} // namespace avalanche

#endif // BITCOIN_AVALANCHE_TEST_UTIL_H
