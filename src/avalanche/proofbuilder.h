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

#ifndef BITCOIN_AVALANCHE_PROOFBUILDER_H
#define BITCOIN_AVALANCHE_PROOFBUILDER_H

#include <avalanche/proof.h>
#include <key.h>

#include <cstdio>

namespace avalanche {

class ProofBuilder {
    uint64_t sequence;
    int64_t expirationTime;
    CPubKey master;

    struct StakeSigner {
        Stake stake;
        CKey key;

        StakeSigner(Stake stake_, CKey key_)
            : stake(std::move(stake_)), key(std::move(key_)) {}

        SignedStake sign(const ProofId &proofid);
    };

    std::vector<StakeSigner> stakes;

public:
    ProofBuilder(uint64_t sequence_, int64_t expirationTime_, CPubKey master_)
        : sequence(sequence_), expirationTime(expirationTime_),
          master(std::move(master_)) {}

    bool addUTXO(COutPoint utxo, Amount amount, uint32_t height,
                 bool is_coinbase, CKey key);

    Proof build();

    /**
     * Builds a randomized (and therefore invalid) Proof.
     * Useful for tests.
     */
    static Proof buildRandom(uint32_t score);

private:
    ProofId getProofId() const;
};

} // namespace avalanche

#endif // BITCOIN_AVALANCHE_PROOFBUILDER_H
