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
// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <zmq/zmqrpc.h>

#include <rpc/server.h>
#include <rpc/util.h>
#include <zmq/zmqabstractnotifier.h>
#include <zmq/zmqnotificationinterface.h>

#include <univalue.h>

namespace {

UniValue getzmqnotifications(const Config &config,
                             const JSONRPCRequest &request) {
    RPCHelpMan{
        "getzmqnotifications",
        "Returns information about the active ZeroMQ notifications.\n",
        {},
        RPCResult{
            RPCResult::Type::ARR,
            "",
            "",
            {
                {RPCResult::Type::OBJ,
                 "",
                 "",
                 {
                     {RPCResult::Type::STR, "type", "Type of notification"},
                     {RPCResult::Type::STR, "address",
                      "Address of the publisher"},
                     {RPCResult::Type::NUM, "hwm",
                      "Outbound message high water mark"},
                 }},
            }},
        RPCExamples{HelpExampleCli("getzmqnotifications", "") +
                    HelpExampleRpc("getzmqnotifications", "")},
    }
        .Check(request);

    UniValue result(UniValue::VARR);
    if (g_zmq_notification_interface != nullptr) {
        for (const auto *n :
             g_zmq_notification_interface->GetActiveNotifiers()) {
            UniValue obj(UniValue::VOBJ);
            obj.pushKV("type", n->GetType());
            obj.pushKV("address", n->GetAddress());
            obj.pushKV("hwm", n->GetOutboundMessageHighWaterMark());
            result.push_back(obj);
        }
    }

    return result;
}

// clang-format off
static const CRPCCommand commands[] = {
    //  category          name                     actor (function)        argNames
    //  ----------------- ------------------------ ----------------------- ----------
    { "zmq",            "getzmqnotifications",   getzmqnotifications,    {} },
};
// clang-format on

} // anonymous namespace

void RegisterZMQRPCCommands(CRPCTable &t) {
    for (const auto &c : commands) {
        t.appendCommand(c.name, &c);
    }
}
