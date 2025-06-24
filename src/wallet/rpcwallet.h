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
// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_RPCWALLET_H
#define BITCOIN_WALLET_RPCWALLET_H

#include <script/sighashtype.h>
#include <span.h>

#include <memory>
#include <string>
#include <vector>

class CRPCCommand;
class CTransaction;
class CWallet;
class Config;
class JSONRPCRequest;
class LegacyScriptPubKeyMan;
struct PartiallySignedTransaction;
class UniValue;
struct WalletContext;

namespace util {
class Ref;
}

Span<const CRPCCommand> GetWalletRPCCommands();

/**
 * Figures out what wallet, if any, to use for a JSONRPCRequest.
 *
 * @param[in] request JSONRPCRequest that wishes to access a wallet
 * @return NULL if no wallet should be used, or a pointer to the CWallet
 */
std::shared_ptr<CWallet>
GetWalletForJSONRPCRequest(const JSONRPCRequest &request);

void EnsureWalletIsUnlocked(const CWallet *);
WalletContext &EnsureWalletContext(const util::Ref &context);
LegacyScriptPubKeyMan &EnsureLegacyScriptPubKeyMan(CWallet &wallet,
                                                   bool also_create = false);

UniValue signrawtransactionwithwallet(const Config &config,
                                      const JSONRPCRequest &request);
UniValue getaddressinfo(const Config &config, const JSONRPCRequest &request);

#endif // BITCOIN_WALLET_RPCWALLET_H
