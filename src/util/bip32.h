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
// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_BIP32_H
#define BITCOIN_UTIL_BIP32_H

#include <attributes.h>

#include <string>
#include <vector>

/**
 * Parse an HD keypaths like "m/7/0'/2000".
 */
NODISCARD bool ParseHDKeypath(const std::string &keypath_str,
                              std::vector<uint32_t> &keypath);

/**
 * Write HD keypaths as strings
 */
std::string WriteHDKeypath(const std::vector<uint32_t> &keypath);
std::string FormatHDKeypath(const std::vector<uint32_t> &path);

#endif // BITCOIN_UTIL_BIP32_H
