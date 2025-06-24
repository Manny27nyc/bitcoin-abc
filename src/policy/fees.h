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
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_POLICY_FEES_H
#define BITCOIN_POLICY_FEES_H

#include <amount.h>
#include <random.h>
#include <uint256.h>

#include <map>
#include <string>
#include <vector>

class CFeeRate;

// Minimum and Maximum values for tracking feerates
static constexpr Amount MIN_FEERATE(10 * SATOSHI);
static const Amount MAX_FEERATE(int64_t(1e7) * SATOSHI);

// We have to lump transactions into buckets based on feerate, but we want to be
// able to give accurate estimates over a large range of potential feerates.
// Therefore it makes sense to exponentially space the buckets
/** Spacing of FeeRate buckets */
static const double FEE_SPACING = 1.1;

class FeeFilterRounder {
public:
    /** Create new FeeFilterRounder */
    explicit FeeFilterRounder(const CFeeRate &minIncrementalFee);

    /** Quantize a minimum fee for privacy purpose before broadcast **/
    Amount round(const Amount currentMinFee);

private:
    std::set<Amount> feeset;
    FastRandomContext insecure_rand;
};

#endif // BITCOIN_POLICY_FEES_H
