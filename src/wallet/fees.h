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
// Copyright (c) 2018-2020 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_FEES_H
#define BITCOIN_WALLET_FEES_H

#include <amount.h>
#include <feerate.h>

class CCoinControl;
class CTxMemPool;
class CWallet;

/**
 * Return the minimum required absolute fee for this size
 * based on the required fee rate
 */
Amount GetRequiredFee(const CWallet &wallet, unsigned int nTxBytes);

/**
 * Estimate the minimum fee considering user set parameters
 * and the required fee
 */
Amount GetMinimumFee(const CWallet &wallet, unsigned int nTxBytes,
                     const CCoinControl &coin_control);

/**
 * Return the minimum required feerate taking into account the
 * minimum relay feerate and user set minimum transaction feerate
 */
CFeeRate GetRequiredFeeRate(const CWallet &wallet);

/**
 * Estimate the minimum fee rate considering user set parameters
 * and the required fee
 */
CFeeRate GetMinimumFeeRate(const CWallet &wallet,
                           const CCoinControl &coin_control);

#endif // BITCOIN_WALLET_FEES_H
