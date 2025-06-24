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

#ifndef BITCOIN_AVALANCHE_DELEGATIONBUILDER_H
#define BITCOIN_AVALANCHE_DELEGATIONBUILDER_H

#include <avalanche/delegation.h>

#include <vector>

class CKey;
class CPubKey;

namespace avalanche {

struct LimitedProofId;
class Proof;

class DelegationBuilder {
    LimitedProofId limitedProofid;
    DelegationId dgid;

    std::vector<Delegation::Level> levels;

    DelegationBuilder(const LimitedProofId &ltdProofId,
                      const CPubKey &proofMaster,
                      const DelegationId &delegationId);

public:
    DelegationBuilder(const LimitedProofId &ltdProofId,
                      const CPubKey &proofMaster);
    explicit DelegationBuilder(const Proof &p);
    explicit DelegationBuilder(const Delegation &dg);

    bool addLevel(const CKey &delegatorKey, const CPubKey &delegatedPubKey);

    Delegation build() const;
};

} // namespace avalanche

#endif // BITCOIN_AVALANCHE_DELEGATIONBUILDER_H
