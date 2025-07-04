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
// Copyright (c) 2018-2020 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BLOCKFILEINFO_H
#define BITCOIN_BLOCKFILEINFO_H

#include <serialize.h>

#include <cstdint>
#include <string>

class CBlockFileInfo {
public:
    //! number of blocks stored in file
    unsigned int nBlocks;
    //! number of used bytes of block file
    unsigned int nSize;
    //! number of used bytes in the undo file
    unsigned int nUndoSize;
    //! lowest height of block in file
    unsigned int nHeightFirst;
    //! highest height of block in file
    unsigned int nHeightLast;
    //! earliest time of block in file
    uint64_t nTimeFirst;
    //! latest time of block in file
    uint64_t nTimeLast;

    SERIALIZE_METHODS(CBlockFileInfo, obj) {
        READWRITE(VARINT(obj.nBlocks));
        READWRITE(VARINT(obj.nSize));
        READWRITE(VARINT(obj.nUndoSize));
        READWRITE(VARINT(obj.nHeightFirst));
        READWRITE(VARINT(obj.nHeightLast));
        READWRITE(VARINT(obj.nTimeFirst));
        READWRITE(VARINT(obj.nTimeLast));
    }

    void SetNull() {
        nBlocks = 0;
        nSize = 0;
        nUndoSize = 0;
        nHeightFirst = 0;
        nHeightLast = 0;
        nTimeFirst = 0;
        nTimeLast = 0;
    }

    CBlockFileInfo() { SetNull(); }

    std::string ToString() const;

    /** update statistics (does not update nSize) */
    void AddBlock(unsigned int nHeightIn, uint64_t nTimeIn) {
        if (nBlocks == 0 || nHeightFirst > nHeightIn) {
            nHeightFirst = nHeightIn;
        }
        if (nBlocks == 0 || nTimeFirst > nTimeIn) {
            nTimeFirst = nTimeIn;
        }
        nBlocks++;
        if (nHeightIn > nHeightLast) {
            nHeightLast = nHeightIn;
        }
        if (nTimeIn > nTimeLast) {
            nTimeLast = nTimeIn;
        }
    }
};

#endif // BITCOIN_BLOCKFILEINFO_H
