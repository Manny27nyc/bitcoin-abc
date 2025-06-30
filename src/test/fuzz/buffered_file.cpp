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
// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <streams.h>

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

void test_one_input(const std::vector<uint8_t> &buffer) {
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FuzzedFileProvider fuzzed_file_provider = ConsumeFile(fuzzed_data_provider);
    std::optional<CBufferedFile> opt_buffered_file;
    FILE *fuzzed_file = fuzzed_file_provider.open();
    try {
        opt_buffered_file.emplace(
            fuzzed_file,
            fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(0, 4096),
            fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(0, 4096),
            fuzzed_data_provider.ConsumeIntegral<int>(),
            fuzzed_data_provider.ConsumeIntegral<int>());
    } catch (const std::ios_base::failure &) {
        if (fuzzed_file != nullptr) {
            fclose(fuzzed_file);
        }
    }
    if (opt_buffered_file && fuzzed_file != nullptr) {
        bool setpos_fail = false;
        while (fuzzed_data_provider.ConsumeBool()) {
            switch (fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 4)) {
                case 0: {
                    std::array<uint8_t, 4096> arr{};
                    try {
                        opt_buffered_file->read(
                            (char *)arr.data(),
                            fuzzed_data_provider.ConsumeIntegralInRange<size_t>(
                                0, 4096));
                    } catch (const std::ios_base::failure &) {
                    }
                    break;
                }
                case 1: {
                    opt_buffered_file->SetLimit(
                        fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(
                            0, 4096));
                    break;
                }
                case 2: {
                    if (!opt_buffered_file->SetPos(
                            fuzzed_data_provider
                                .ConsumeIntegralInRange<uint64_t>(0, 4096))) {
                        setpos_fail = true;
                    }
                    break;
                }
                case 3: {
                    if (setpos_fail) {
                        // Calling FindByte(...) after a failed SetPos(...) call
                        // may result in an infinite loop.
                        break;
                    }
                    try {
                        opt_buffered_file->FindByte(
                            fuzzed_data_provider.ConsumeIntegral<char>());
                    } catch (const std::ios_base::failure &) {
                    }
                    break;
                }
                case 4: {
                    ReadFromStream(fuzzed_data_provider, *opt_buffered_file);
                    break;
                }
            }
        }
        opt_buffered_file->GetPos();
        opt_buffered_file->GetType();
        opt_buffered_file->GetVersion();
    }
}
