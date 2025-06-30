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
// Copyright (c) 2013-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NOUI_H
#define BITCOIN_NOUI_H

#include <string>

struct bilingual_str;

/** Non-GUI handler, which logs and prints messages. */
bool noui_ThreadSafeMessageBox(const bilingual_str &message,
                               const std::string &caption, unsigned int style);
/** Non-GUI handler, which logs and prints questions. */
bool noui_ThreadSafeQuestion(
    const bilingual_str & /* ignored interactive message */,
    const std::string &message, const std::string &caption, unsigned int style);
/** Non-GUI handler, which only logs a message. */
void noui_InitMessage(const std::string &message);

/** Connect all bitcoind signal handlers */
void noui_connect();

/**
 * Redirect all bitcoind signal handlers to LogPrintf. Used to check or suppress
 * output during test runs that produce expected errors.
 */
void noui_test_redirect();

/**
 * Reconnects the regular Non-GUI handlers after having used noui_test_redirect.
 */
void noui_reconnect();

#endif // BITCOIN_NOUI_H
