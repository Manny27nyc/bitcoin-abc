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
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ADDRDB_H
#define BITCOIN_ADDRDB_H

#include <fs.h>
#include <net_types.h> // For banmap_t
#include <serialize.h>

#include <map>
#include <string>

class CSubNet;
class CAddrMan;
class CDataStream;
class CChainParams;

class CBanEntry {
public:
    static const int CURRENT_VERSION = 1;
    int nVersion;
    int64_t nCreateTime;
    int64_t nBanUntil;

    CBanEntry() { SetNull(); }

    explicit CBanEntry(int64_t nCreateTimeIn) {
        SetNull();
        nCreateTime = nCreateTimeIn;
    }

    SERIALIZE_METHODS(CBanEntry, obj) {
        //! For backward compatibility
        uint8_t ban_reason = 2;
        READWRITE(obj.nVersion, obj.nCreateTime, obj.nBanUntil, ban_reason);
    }

    void SetNull() {
        nVersion = CBanEntry::CURRENT_VERSION;
        nCreateTime = 0;
        nBanUntil = 0;
    }
};

/** Access to the (IP) address database (peers.dat) */
class CAddrDB {
private:
    fs::path pathAddr;
    const CChainParams &chainParams;

public:
    explicit CAddrDB(const CChainParams &chainParams);
    bool Write(const CAddrMan &addr);
    bool Read(CAddrMan &addr);
    bool Read(CAddrMan &addr, CDataStream &ssPeers);
};

/** Access to the banlist database (banlist.dat) */
class CBanDB {
private:
    const fs::path m_ban_list_path;
    const CChainParams &chainParams;

public:
    CBanDB(fs::path ban_list_path, const CChainParams &_chainParams);
    bool Write(const banmap_t &banSet);
    bool Read(banmap_t &banSet);
};

#endif // BITCOIN_ADDRDB_H
