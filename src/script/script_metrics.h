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
// Copyright (c) 2020 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_SCRIPT_METRICS_H
#define BITCOIN_SCRIPT_SCRIPT_METRICS_H

/**
 * Struct for holding cumulative results from executing a script or a sequence
 * of scripts.
 */
struct ScriptExecutionMetrics {
    int nSigChecks = 0;
};

#endif // BITCOIN_SCRIPT_SCRIPT_METRICS_H
