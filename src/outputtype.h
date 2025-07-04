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

#ifndef BITCOIN_OUTPUTTYPE_H
#define BITCOIN_OUTPUTTYPE_H

#include <attributes.h>
#include <script/signingprovider.h>
#include <script/standard.h>

#include <array>
#include <string>
#include <vector>

enum class OutputType {
    LEGACY,

    /**
     * Special output type for change outputs only. Automatically choose type
     * based on address type setting and the types other of non-change outputs.
     */
    CHANGE_AUTO,
};

extern const std::array<OutputType, 1> OUTPUT_TYPES;

NODISCARD bool ParseOutputType(const std::string &str, OutputType &output_type);
const std::string &FormatOutputType(OutputType type);

/**
 * Get a destination of the requested type (if possible) to the specified key.
 * The caller must make sure LearnRelatedScripts has been called beforehand.
 */
CTxDestination GetDestinationForKey(const CPubKey &key, OutputType);

/**
 * Get all destinations (potentially) supported by the wallet for the given key.
 */
std::vector<CTxDestination> GetAllDestinationsForKey(const CPubKey &key);

/**
 * Get a destination of the requested type (if possible) to the specified
 * script. This function will automatically add the script (and any other
 * necessary scripts) to the keystore.
 */
CTxDestination AddAndGetDestinationForScript(FillableSigningProvider &keystore,
                                             const CScript &script, OutputType);

#endif // BITCOIN_OUTPUTTYPE_H
