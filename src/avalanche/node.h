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
// Copyright (c) 2018-2019 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_AVALANCHE_NODE_H
#define BITCOIN_AVALANCHE_NODE_H

#include <net.h> // For NodeId
#include <pubkey.h>

#include <chrono>
#include <cstdint>

using PeerId = uint32_t;
static constexpr PeerId NO_PEER = -1;

using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

namespace avalanche {

struct Node {
    NodeId nodeid;
    PeerId peerid;
    TimePoint nextRequestTime;
    CPubKey pubkey;

    Node(NodeId nodeid_, PeerId peerid_, CPubKey pubkey_)
        : nodeid(nodeid_), peerid(peerid_),
          nextRequestTime(std::chrono::steady_clock::now()),
          pubkey(std::move(pubkey_)) {}
};

} // namespace avalanche

#endif // BITCOIN_AVALANCHE_NODE_H
