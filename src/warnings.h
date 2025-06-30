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
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WARNINGS_H
#define BITCOIN_WARNINGS_H

#include <string>

struct bilingual_str;

void SetMiscWarning(const bilingual_str &strWarning);

void SetfLargeWorkForkFound(bool flag);
bool GetfLargeWorkForkFound();
void SetfLargeWorkInvalidChainFound(bool flag);
/**
 * Format a string that describes several potential problems detected by the
 * core.
 * @param[in] verbose bool
 * - if true, get all warnings separated by <hr />
 * - if false, get the most important warning
 * @returns the warning string
 * This function only returns the highest priority warning of the set selected
 * by strFor.
 */
bilingual_str GetWarnings(bool verbose);

#endif // BITCOIN_WARNINGS_H
