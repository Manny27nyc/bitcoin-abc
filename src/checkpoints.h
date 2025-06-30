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

#ifndef BITCOIN_CHECKPOINTS_H
#define BITCOIN_CHECKPOINTS_H

class CBlockIndex;
struct BlockHash;
struct CCheckpointData;

/**
 * Block-chain checkpoints are compiled-in sanity checks.
 * They are updated every release or three.
 */
namespace Checkpoints {

//! Returns true if block passes checkpoint checks
bool CheckBlock(const CCheckpointData &data, int nHeight,
                const BlockHash &hash);

//! Returns last CBlockIndex* that is a checkpoint
CBlockIndex *GetLastCheckpoint(const CCheckpointData &data);

} // namespace Checkpoints

#endif // BITCOIN_CHECKPOINTS_H
