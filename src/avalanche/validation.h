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

#ifndef BITCOIN_AVALANCHE_VALIDATION_H
#define BITCOIN_AVALANCHE_VALIDATION_H

#include <consensus/validation.h>

namespace avalanche {

enum class ProofValidationResult {
    NONE = 0,
    NO_STAKE,
    DUST_THRESOLD,
    DUPLICATE_STAKE,
    INVALID_SIGNATURE,
    TOO_MANY_UTXOS,

    // UTXO based errors.
    MISSING_UTXO,
    COINBASE_MISMATCH,
    HEIGHT_MISMATCH,
    AMOUNT_MISMATCH,
    NON_STANDARD_DESTINATION,
    DESTINATION_NOT_SUPPORTED,
    DESTINATION_MISMATCH,
};

class ProofValidationState : public ValidationState<ProofValidationResult> {};

enum class DelegationResult {
    NONE = 0,
    INVALID_SIGNATURE,
};

class DelegationState : public ValidationState<DelegationResult> {};

} // namespace avalanche

#endif // BITCOIN_AVALANCHE_VALIDATION_H
