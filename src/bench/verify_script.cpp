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
// Copyright (c) 2016-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <key.h>
#if defined(HAVE_CONSENSUS_LIB)
#include <script/bitcoinconsensus.h>
#endif
#include <script/interpreter.h>
#include <script/script.h>
#include <script/script_error.h>
#include <script/standard.h>
#include <streams.h>
#include <test/util/transaction_utils.h>

#include <array>

static void VerifyNestedIfScript(benchmark::Bench &bench) {
    std::vector<std::vector<uint8_t>> stack;
    CScript script;
    for (int i = 0; i < 100; ++i) {
        script << OP_1 << OP_IF;
    }
    for (int i = 0; i < 1000; ++i) {
        script << OP_1;
    }
    for (int i = 0; i < 100; ++i) {
        script << OP_ENDIF;
    }
    bench.run([&] {
        auto stack_copy = stack;
        ScriptExecutionMetrics metrics = {};
        ScriptError error;
        bool ret = EvalScript(stack_copy, script, 0, BaseSignatureChecker(),
                              metrics, &error);
        assert(ret);
    });
}

BENCHMARK(VerifyNestedIfScript);
