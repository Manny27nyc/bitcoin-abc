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
// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/script.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <string>
#include <vector>

void test_one_input(const std::vector<uint8_t> &buffer) {
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    CScript script = ConsumeScript(fuzzed_data_provider);
    while (fuzzed_data_provider.remaining_bytes() > 0) {
        switch (fuzzed_data_provider.ConsumeIntegralInRange(0, 7)) {
            case 0: {
                CScript s = ConsumeScript(fuzzed_data_provider);
                script = std::move(s);
                break;
            }
            case 1: {
                const CScript &s = ConsumeScript(fuzzed_data_provider);
                script = s;
                break;
            }
            case 2:
                script << fuzzed_data_provider.ConsumeIntegral<int64_t>();
                break;
            case 3:
                script << ConsumeOpcodeType(fuzzed_data_provider);
                break;
            case 4:
                script << ConsumeScriptNum(fuzzed_data_provider);
                break;
            case 5:
                script << ConsumeRandomLengthByteVector(fuzzed_data_provider);
                break;
            case 6:
                script.clear();
                break;
            case 7: {
                (void)script.IsWitnessProgram();
                (void)script.HasValidOps();
                (void)script.IsPayToScriptHash();
                (void)script.IsPushOnly();
                (void)script.IsUnspendable();
                {
                    CScript::const_iterator pc = script.begin();
                    opcodetype opcode;
                    (void)script.GetOp(pc, opcode);
                    std::vector<uint8_t> data;
                    (void)script.GetOp(pc, opcode, data);
                    (void)script.IsPushOnly(pc);
                }
                {
                    int version;
                    std::vector<uint8_t> program;
                    (void)script.IsWitnessProgram(version, program);
                }
                break;
            }
        }
    }
}
