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
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <checkpoints.h>

#include <chainparams.h>
#include <reverse_iterator.h>
// D6970 moved LookupBlockIndex from chain.h to validation.h TODO: remove this
// when LookupBlockIndex is refactored out of validation
#include <validation.h>

#include <cstdint>

namespace Checkpoints {

bool CheckBlock(const CCheckpointData &data, int nHeight,
                const BlockHash &hash) {
    const MapCheckpoints &checkpoints = data.mapCheckpoints;

    MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
    if (i == checkpoints.end()) {
        return true;
    }
    return hash == i->second;
}

CBlockIndex *GetLastCheckpoint(const CCheckpointData &data)
    EXCLUSIVE_LOCKS_REQUIRED(cs_main) {
    const MapCheckpoints &checkpoints = data.mapCheckpoints;

    for (const MapCheckpoints::value_type &i : reverse_iterate(checkpoints)) {
        const BlockHash &hash = i.second;
        CBlockIndex *pindex = LookupBlockIndex(hash);
        if (pindex) {
            return pindex;
        }
    }

    return nullptr;
}

} // namespace Checkpoints
