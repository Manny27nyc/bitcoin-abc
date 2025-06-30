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

#include <consensus/validation.h>
#include <core_memusage.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <version.h>

#include <test/fuzz/fuzz.h>

void test_one_input(const std::vector<uint8_t> &buffer) {
    CDataStream ds(buffer, SER_NETWORK, INIT_PROTO_VERSION);
    CTxOut tx_out;
    try {
        int version;
        ds >> version;
        ds.SetVersion(version);
        ds >> tx_out;
    } catch (const std::ios_base::failure &) {
        return;
    }

    const CFeeRate dust_relay_fee{DUST_RELAY_TX_FEE};
    (void)GetDustThreshold(tx_out, dust_relay_fee);
    (void)IsDust(tx_out, dust_relay_fee);
    (void)RecursiveDynamicUsage(tx_out);

    (void)tx_out.ToString();
    (void)tx_out.IsNull();
    tx_out.SetNull();
    assert(tx_out.IsNull());
}
