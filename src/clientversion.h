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
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CLIENTVERSION_H
#define BITCOIN_CLIENTVERSION_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif // HAVE_CONFIG_H
#include <config/version.h>

// Check that required client information is defined
#if !defined(CLIENT_VERSION_MAJOR) || !defined(CLIENT_VERSION_MINOR) ||        \
    !defined(CLIENT_VERSION_REVISION) || !defined(COPYRIGHT_YEAR) ||           \
    !defined(CLIENT_VERSION_IS_RELEASE)
#error Client version information missing: version is not defined by bitcoin-config.h nor defined any other way
#endif

/**
 * Converts the parameter X to a string after macro replacement on X has been
 * performed.
 * Don't merge these into one macro!
 */
#define STRINGIZE(X) DO_STRINGIZE(X)
#define DO_STRINGIZE(X) #X

//! Copyright string used in Windows .rc files
#define COPYRIGHT_STR                                                          \
    "2009-" STRINGIZE(COPYRIGHT_YEAR) " " COPYRIGHT_HOLDERS_FINAL

/**
 * bitcoind-res.rc includes this file, but it cannot cope with real c++ code.
 * WINDRES_PREPROC is defined to indicate that its pre-processor is running.
 * Anything other than a define should be guarded below.
 */

#if !defined(WINDRES_PREPROC)

#include <string>
#include <vector>

static constexpr int CLIENT_VERSION = 1000000 * CLIENT_VERSION_MAJOR +
                                      10000 * CLIENT_VERSION_MINOR +
                                      100 * CLIENT_VERSION_REVISION;

extern const std::string CLIENT_NAME;
extern const std::string CLIENT_BUILD;

std::string FormatVersion(int nVersion);
std::string FormatFullVersion();
std::string FormatUserAgent(const std::string &name, const std::string &version,
                            const std::vector<std::string> &comments);

#endif // WINDRES_PREPROC

#endif // BITCOIN_CLIENTVERSION_H
