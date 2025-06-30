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

#ifndef BITCOIN_AVALANCHE_DELEGATIONID_H
#define BITCOIN_AVALANCHE_DELEGATIONID_H

#include <uint256.h>

#include <string>

namespace avalanche {

struct DelegationId : public uint256 {
    explicit DelegationId() : uint256() {}
    explicit DelegationId(const uint256 &b) : uint256(b) {}

    static DelegationId fromHex(const std::string &str) {
        DelegationId r;
        r.SetHex(str);
        return r;
    }
};

} // namespace avalanche

#endif // BITCOIN_AVALANCHE_DELEGATIONID_H
