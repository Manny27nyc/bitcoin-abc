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
// Copyright (c) 2016-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/data.h>

#include <rpc/blockchain.h>
#include <streams.h>
#include <validation.h>

#include <univalue.h>

static void BlockToJsonVerbose(benchmark::Bench &bench) {
    CDataStream stream(benchmark::data::block413567, SER_NETWORK,
                       PROTOCOL_VERSION);
    char a = '\0';
    // Prevent compaction
    stream.write(&a, 1);

    CBlock block;
    stream >> block;

    CBlockIndex blockindex;
    const auto blockHash = block.GetHash();
    blockindex.phashBlock = &blockHash;
    blockindex.nBits = 403014710;

    bench.run([&] {
        (void)blockToJSON(block, &blockindex, &blockindex, /*verbose*/ true);
    });
}

BENCHMARK(BlockToJsonVerbose);
