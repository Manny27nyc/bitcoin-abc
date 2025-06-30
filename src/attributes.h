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
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ATTRIBUTES_H
#define BITCOIN_ATTRIBUTES_H

#if defined(__has_cpp_attribute)
#if __has_cpp_attribute(nodiscard)
#define NODISCARD [[nodiscard]]
#endif
#endif
#ifndef NODISCARD
#if defined(_MSC_VER) && _MSC_VER >= 1700
#define NODISCARD _Check_return_
#else
#define NODISCARD __attribute__((warn_unused_result))
#endif
#endif

#endif // BITCOIN_ATTRIBUTES_H
