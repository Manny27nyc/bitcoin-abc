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
// Copyright (c) 2021 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <dnsseeds.h>

#include <random.h>
#include <util/system.h>

const std::vector<std::string>
GetRandomizedDNSSeeds(const CChainParams &params) {
    FastRandomContext rng;
    std::vector<std::string> seeds;
    if (gArgs.IsArgSet("-overridednsseed")) {
        seeds = {gArgs.GetArg("-overridednsseed", "")};
    } else {
        seeds = params.vSeeds;
    }

    Shuffle(seeds.begin(), seeds.end(), rng);
    return seeds;
}
