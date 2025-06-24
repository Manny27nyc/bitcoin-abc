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

#ifndef BITCOIN_DNSSEEDS_H
#define BITCOIN_DNSSEEDS_H

#include <chainparams.h>

#include <string>
#include <vector>

/** Return the list of hostnames to look up for DNS seeds */
const std::vector<std::string>
GetRandomizedDNSSeeds(const CChainParams &params);

#endif // BITCOIN_DNSSEEDS_H
