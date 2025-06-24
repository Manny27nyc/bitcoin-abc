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

#ifndef BITCOIN_AVALANCHE_PROOFID_H
#define BITCOIN_AVALANCHE_PROOFID_H

#include <salteduint256hasher.h>
#include <uint256.h>

#include <string>

class CPubKey;

namespace avalanche {

struct ProofId : public uint256 {
    explicit ProofId() : uint256() {}
    explicit ProofId(const uint256 &b) : uint256(b) {}

    static ProofId fromHex(const std::string &str) {
        ProofId r;
        r.SetHex(str);
        return r;
    }
};

struct LimitedProofId : public uint256 {
    explicit LimitedProofId() : uint256() {}
    explicit LimitedProofId(const uint256 &b) : uint256(b) {}

    static LimitedProofId fromHex(const std::string &str) {
        LimitedProofId r;
        r.SetHex(str);
        return r;
    }

    ProofId computeProofId(const CPubKey &proofMaster) const;
};

class SaltedProofIdHasher : private SaltedUint256Hasher {
public:
    SaltedProofIdHasher() : SaltedUint256Hasher() {}
    size_t operator()(const ProofId &proofid) const { return hash(proofid); }
};

} // namespace avalanche

#endif // BITCOIN_AVALANCHE_PROOFID_H
