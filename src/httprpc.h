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
// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_HTTPRPC_H
#define BITCOIN_HTTPRPC_H

#include <httpserver.h>
#include <rpc/server.h>

class Config;

namespace util {
class Ref;
} // namespace util

class HTTPRPCRequestProcessor {
private:
    Config &config;
    RPCServer &rpcServer;

    bool ProcessHTTPRequest(HTTPRequest *request);

public:
    const util::Ref &context;

    HTTPRPCRequestProcessor(Config &configIn, RPCServer &rpcServerIn,
                            const util::Ref &contextIn)
        : config(configIn), rpcServer(rpcServerIn), context(contextIn) {}

    static bool DelegateHTTPRequest(HTTPRPCRequestProcessor *requestProcessor,
                                    HTTPRequest *request) {
        return requestProcessor->ProcessHTTPRequest(request);
    }
};

/**
 * Start HTTP RPC subsystem.
 * Precondition; HTTP and RPC has been started.
 */
bool StartHTTPRPC(HTTPRPCRequestProcessor &httpRPCRequestProcessor);

/** Interrupt HTTP RPC subsystem */
void InterruptHTTPRPC();

/**
 * Stop HTTP RPC subsystem.
 * Precondition; HTTP and RPC has been stopped.
 */
void StopHTTPRPC();

/**
 * Start HTTP REST subsystem.
 * Precondition; HTTP and RPC has been started.
 */
void StartREST(const util::Ref &context);

/** Interrupt RPC REST subsystem */
void InterruptREST();

/**
 * Stop HTTP REST subsystem.
 * Precondition; HTTP and RPC has been stopped.
 */
void StopREST();

#endif // BITCOIN_HTTPRPC_H
