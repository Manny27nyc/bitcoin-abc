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
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <core_io.h>
#include <rpc/client.h>
#include <rpc/util.h>

#include <test/fuzz/fuzz.h>

#include <limits>
#include <string>

void initialize() {
    static const ECCVerifyHandle verify_handle;
    SelectParams(CBaseChainParams::REGTEST);
}

void test_one_input(const std::vector<uint8_t> &buffer) {
    const std::string random_string(buffer.begin(), buffer.end());
    bool valid = true;
    const UniValue univalue = [&] {
        try {
            return ParseNonRFCJSONValue(random_string);
        } catch (const std::runtime_error &) {
            valid = false;
            return NullUniValue;
        }
    }();
    if (!valid) {
        return;
    }
    try {
        (void)ParseHashO(univalue, "A");
    } catch (const UniValue &) {
    } catch (const std::runtime_error &) {
    }
    try {
        (void)ParseHashO(univalue, random_string);
    } catch (const UniValue &) {
    } catch (const std::runtime_error &) {
    }
    try {
        (void)ParseHashV(univalue, "A");
    } catch (const UniValue &) {
    } catch (const std::runtime_error &) {
    }
    try {
        (void)ParseHashV(univalue, random_string);
    } catch (const UniValue &) {
    } catch (const std::runtime_error &) {
    }
    try {
        (void)ParseHexO(univalue, "A");
    } catch (const UniValue &) {
    }
    try {
        (void)ParseHexO(univalue, random_string);
    } catch (const UniValue &) {
    }
    try {
        (void)ParseHexUV(univalue, "A");
        (void)ParseHexUV(univalue, random_string);
    } catch (const UniValue &) {
    } catch (const std::runtime_error &) {
    }
    try {
        (void)ParseHexV(univalue, "A");
    } catch (const UniValue &) {
    } catch (const std::runtime_error &) {
    }
    try {
        (void)ParseHexV(univalue, random_string);
    } catch (const UniValue &) {
    } catch (const std::runtime_error &) {
    }
    try {
        (void)ParseSighashString(univalue);
    } catch (const std::runtime_error &) {
    }
    try {
        (void)AmountFromValue(univalue);
    } catch (const UniValue &) {
    } catch (const std::runtime_error &) {
    }
    try {
        FlatSigningProvider provider;
        (void)EvalDescriptorStringOrObject(univalue, provider);
    } catch (const UniValue &) {
    } catch (const std::runtime_error &) {
    }
    try {
        (void)ParseDescriptorRange(univalue);
    } catch (const UniValue &) {
    } catch (const std::runtime_error &) {
    }
}
