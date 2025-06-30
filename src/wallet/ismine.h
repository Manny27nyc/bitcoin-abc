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

#ifndef BITCOIN_WALLET_ISMINE_H
#define BITCOIN_WALLET_ISMINE_H

#include <script/standard.h>

#include <bitset>
#include <cstdint>

class CWallet;
class CScript;

/** IsMine() return codes */
enum isminetype : unsigned int {
    ISMINE_NO = 0,
    ISMINE_WATCH_ONLY = 1 << 0,
    ISMINE_SPENDABLE = 1 << 1,
    ISMINE_USED = 1 << 2,
    ISMINE_ALL = ISMINE_WATCH_ONLY | ISMINE_SPENDABLE,
    ISMINE_ALL_USED = ISMINE_ALL | ISMINE_USED,
    ISMINE_ENUM_ELEMENTS,
};

/** used for bitflags of isminetype */
typedef uint8_t isminefilter;

/**
 * Cachable amount subdivided into watchonly and spendable parts.
 */
struct CachableAmount {
    // NO and ALL are never (supposed to be) cached
    std::bitset<ISMINE_ENUM_ELEMENTS> m_cached;
    Amount m_value[ISMINE_ENUM_ELEMENTS];
    inline void Reset() { m_cached.reset(); }
    void Set(isminefilter filter, Amount value) {
        m_cached.set(filter);
        m_value[filter] = value;
    }
};

#endif // BITCOIN_WALLET_ISMINE_H
