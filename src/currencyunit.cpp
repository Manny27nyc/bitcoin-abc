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
// Copyright (c) 2021 The Bitcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <currencyunit.h>

#include <util/system.h>

void SetupCurrencyUnitOptions(ArgsManager &argsman) {
    // whether to use eCash default unit and address prefix
    argsman.AddArg("-ecash",
                   strprintf("Use the eCash prefixes and units (default: %s)",
                             DEFAULT_ECASH ? "true" : "false"),
                   ArgsManager::ALLOW_BOOL, OptionsCategory::OPTIONS);
}
