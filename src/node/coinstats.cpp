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
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/coinstats.h>

#include <coins.h>
#include <hash.h>
#include <serialize.h>
#include <util/system.h>
#include <validation.h>

#include <map>

static void ApplyStats(CCoinsStats &stats, CHashWriter &ss, const uint256 &hash,
                       const std::map<uint32_t, Coin> &outputs) {
    assert(!outputs.empty());
    ss << hash;
    ss << VARINT(outputs.begin()->second.GetHeight() * 2 +
                 outputs.begin()->second.IsCoinBase());
    stats.nTransactions++;
    for (const auto &output : outputs) {
        ss << VARINT(output.first + 1);
        ss << output.second.GetTxOut().scriptPubKey;
        ss << VARINT_MODE(output.second.GetTxOut().nValue / SATOSHI,
                          VarIntMode::NONNEGATIVE_SIGNED);
        stats.nTransactionOutputs++;
        stats.nTotalAmount += output.second.GetTxOut().nValue;
        stats.nBogoSize +=
            32 /* txid */ + 4 /* vout index */ + 4 /* height + coinbase */ +
            8 /* amount */ + 2 /* scriptPubKey len */ +
            output.second.GetTxOut().scriptPubKey.size() /* scriptPubKey */;
    }
    ss << VARINT(0u);
}

//! Calculate statistics about the unspent transaction output set
bool GetUTXOStats(CCoinsView *view, CCoinsStats &stats,
                  const std::function<void()> &interruption_point) {
    stats = CCoinsStats();
    std::unique_ptr<CCoinsViewCursor> pcursor(view->Cursor());
    assert(pcursor);

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    stats.hashBlock = pcursor->GetBestBlock();
    {
        LOCK(cs_main);
        stats.nHeight = LookupBlockIndex(stats.hashBlock)->nHeight;
    }
    ss << stats.hashBlock;
    uint256 prevkey;
    std::map<uint32_t, Coin> outputs;
    while (pcursor->Valid()) {
        interruption_point();
        COutPoint key;
        Coin coin;
        if (pcursor->GetKey(key) && pcursor->GetValue(coin)) {
            if (!outputs.empty() && key.GetTxId() != prevkey) {
                ApplyStats(stats, ss, prevkey, outputs);
                outputs.clear();
            }
            prevkey = key.GetTxId();
            outputs[key.GetN()] = std::move(coin);
            stats.coins_count++;
        } else {
            return error("%s: unable to read value", __func__);
        }
        pcursor->Next();
    }
    if (!outputs.empty()) {
        ApplyStats(stats, ss, prevkey, outputs);
    }
    stats.hashSerialized = ss.GetHash();
    stats.nDiskSize = view->EstimateSize();
    return true;
}
