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
// Copyright (c) 2018-2019 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/protocol.h>
#include <rpc/request.h>
#include <rpc/server.h>

#include <chainparams.h>
#include <config.h>
#include <util/ref.h>
#include <util/system.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <string>

BOOST_FIXTURE_TEST_SUITE(rpc_server_tests, TestingSetup)

class ArgsTestRPCCommand : public RPCCommandWithArgsContext {
public:
    explicit ArgsTestRPCCommand(const std::string &nameIn)
        : RPCCommandWithArgsContext(nameIn) {}

    UniValue Execute(const UniValue &args) const override {
        BOOST_CHECK_EQUAL(args["arg1"].get_str(), "value1");
        return UniValue("testing1");
    }
};

static bool isRpcMethodNotFound(const UniValue &u) {
    return find_value(u, "code").get_int() == int(RPC_METHOD_NOT_FOUND);
}

BOOST_AUTO_TEST_CASE(rpc_server_execute_command) {
    DummyConfig config;
    RPCServer rpcServer;
    const std::string commandName = "testcommand1";
    rpcServer.RegisterCommand(
        std::make_unique<ArgsTestRPCCommand>(commandName));

    UniValue args(UniValue::VOBJ);
    args.pushKV("arg1", "value1");

    // Registered commands execute and return values correctly
    util::Ref context{m_node};
    JSONRPCRequest request(context);
    request.strMethod = commandName;
    request.params = args;
    UniValue output = rpcServer.ExecuteCommand(config, request);
    BOOST_CHECK_EQUAL(output.get_str(), "testing1");

    // Not-registered commands throw an exception as expected
    JSONRPCRequest badCommandRequest(context);
    badCommandRequest.strMethod = "this-command-does-not-exist";
    BOOST_CHECK_EXCEPTION(rpcServer.ExecuteCommand(config, badCommandRequest),
                          UniValue, isRpcMethodNotFound);
}

class RequestContextRPCCommand : public RPCCommand {
public:
    explicit RequestContextRPCCommand(const std::string &nameIn)
        : RPCCommand(nameIn) {}

    // Sanity check that Execute(JSONRPCRequest) is called correctly from
    // RPCServer
    UniValue Execute(const JSONRPCRequest &request) const override {
        const UniValue args = request.params;
        BOOST_CHECK_EQUAL(request.strMethod, "testcommand2");
        BOOST_CHECK_EQUAL(args["arg2"].get_str(), "value2");
        return UniValue("testing2");
    }
};

BOOST_AUTO_TEST_CASE(rpc_server_execute_command_from_request_context) {
    DummyConfig config;
    RPCServer rpcServer;
    const std::string commandName = "testcommand2";
    rpcServer.RegisterCommand(
        std::make_unique<RequestContextRPCCommand>(commandName));

    UniValue args(UniValue::VOBJ);
    args.pushKV("arg2", "value2");

    // Registered commands execute and return values correctly
    util::Ref context{m_node};
    JSONRPCRequest request(context);
    request.strMethod = commandName;
    request.params = args;
    UniValue output = rpcServer.ExecuteCommand(config, request);
    BOOST_CHECK_EQUAL(output.get_str(), "testing2");
}

BOOST_AUTO_TEST_SUITE_END()
