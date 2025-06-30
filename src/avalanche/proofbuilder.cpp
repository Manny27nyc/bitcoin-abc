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

#include <avalanche/proofbuilder.h>

#include <random.h>

namespace avalanche {

SignedStake ProofBuilder::StakeSigner::sign(const ProofId &proofid) {
    const uint256 h = stake.getHash(proofid);

    SchnorrSig sig;
    if (!key.SignSchnorr(h, sig)) {
        sig.fill(0);
    }

    return SignedStake(std::move(stake), std::move(sig));
}

bool ProofBuilder::addUTXO(COutPoint utxo, Amount amount, uint32_t height,
                           bool is_coinbase, CKey key) {
    if (!key.IsValid()) {
        return false;
    }

    stakes.emplace_back(
        Stake(std::move(utxo), amount, height, is_coinbase, key.GetPubKey()),
        std::move(key));
    return true;
}

Proof ProofBuilder::build() {
    const ProofId proofid = getProofId();

    std::vector<SignedStake> signedStakes;
    signedStakes.reserve(stakes.size());

    for (auto &s : stakes) {
        signedStakes.push_back(s.sign(proofid));
    }

    stakes.clear();
    return Proof(sequence, expirationTime, std::move(master),
                 std::move(signedStakes));
}

ProofId ProofBuilder::getProofId() const {
    CHashWriter ss(SER_GETHASH, 0);
    ss << sequence;
    ss << expirationTime;

    WriteCompactSize(ss, stakes.size());
    for (const auto &s : stakes) {
        ss << s.stake;
    }

    CHashWriter ss2(SER_GETHASH, 0);
    ss2 << ss.GetHash();
    ss2 << master;

    return ProofId(ss2.GetHash());
}

Proof ProofBuilder::buildRandom(uint32_t score) {
    CKey key;
    key.MakeNewKey(true);

    ProofBuilder pb(0, std::numeric_limits<uint32_t>::max(), CPubKey());
    pb.addUTXO(COutPoint(TxId(GetRandHash()), 0), (int64_t(score) * COIN) / 100,
               0, false, std::move(key));
    return pb.build();
}

} // namespace avalanche
