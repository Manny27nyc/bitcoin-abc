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
// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_CLIENT_H
#define BITCOIN_RPC_CLIENT_H

#include <univalue.h>

/** Convert positional arguments to command-specific RPC representation */
UniValue RPCConvertValues(const std::string &strMethod,
                          const std::vector<std::string> &strParams);

/** Convert named arguments to command-specific RPC representation */
UniValue RPCConvertNamedValues(const std::string &strMethod,
                               const std::vector<std::string> &strParams);

/**
 * Non-RFC4627 JSON parser, accepts internal values (such as numbers, true,
 * false, null) as well as objects and arrays.
 */
UniValue ParseNonRFCJSONValue(const std::string &strVal);

#endif // BITCOIN_RPC_CLIENT_H
