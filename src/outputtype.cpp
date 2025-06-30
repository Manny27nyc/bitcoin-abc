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
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <outputtype.h>

#include <pubkey.h>
#include <script/script.h>
#include <script/sign.h>
#include <util/vector.h>

#include <cassert>

static const std::string OUTPUT_TYPE_STRING_LEGACY = "legacy";

const std::array<OutputType, 1> OUTPUT_TYPES = {{OutputType::LEGACY}};

bool ParseOutputType(const std::string &type, OutputType &output_type) {
    if (type == OUTPUT_TYPE_STRING_LEGACY) {
        output_type = OutputType::LEGACY;
        return true;
    }
    return false;
}

const std::string &FormatOutputType(OutputType type) {
    switch (type) {
        case OutputType::LEGACY:
            return OUTPUT_TYPE_STRING_LEGACY;
        default:
            assert(false);
    }
}

CTxDestination GetDestinationForKey(const CPubKey &key, OutputType type) {
    switch (type) {
        case OutputType::LEGACY:
            return PKHash(key);
        default:
            assert(false);
    }
}

std::vector<CTxDestination> GetAllDestinationsForKey(const CPubKey &key) {
    PKHash keyid(key);
    CTxDestination p2pkh{keyid};
    return Vector(std::move(p2pkh));
}

CTxDestination AddAndGetDestinationForScript(FillableSigningProvider &keystore,
                                             const CScript &script,
                                             OutputType type) {
    // Add script to keystore
    keystore.AddCScript(script);
    // Note that scripts over 520 bytes are not yet supported.
    switch (type) {
        case OutputType::LEGACY:
            return ScriptHash(script);
        default:
            assert(false);
    }
}
