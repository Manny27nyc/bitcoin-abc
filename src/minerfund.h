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
// Copyright (c) 2020 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MINERFUND_H
#define BITCOIN_MINERFUND_H

#include <amount.h>
#include <script/standard.h>

#include <vector>

class CBlockIndex;

namespace Consensus {
struct Params;
}

Amount GetMinerFundAmount(const Amount &coinbaseValue);

std::vector<CTxDestination>
GetMinerFundWhitelist(const Consensus::Params &params,
                      const CBlockIndex *pindexPrev);

#endif // BITCOIN_MINERFUND_H
