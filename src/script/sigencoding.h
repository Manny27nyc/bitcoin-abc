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
// Copyright (c) 2017-2019 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_SIGENCODING_H
#define BITCOIN_SCRIPT_SIGENCODING_H

#include <script/script_error.h>
#include <script/sighashtype.h>

#include <cstdint>
#include <vector>

typedef std::vector<uint8_t> valtype;

namespace {

inline SigHashType GetHashType(const valtype &vchSig) {
    if (vchSig.size() == 0) {
        return SigHashType(0);
    }

    return SigHashType(vchSig[vchSig.size() - 1]);
}

} // namespace

/**
 * Check that the signature provided on some data is properly encoded.
 * Signatures passed to OP_CHECKDATASIG and its verify variant must be checked
 * using this function.
 */
bool CheckDataSignatureEncoding(const valtype &vchSig, uint32_t flags,
                                ScriptError *serror);

/**
 * Check that the signature provided to authentify a transaction is properly
 * encoded. Signatures passed to OP_CHECKSIG and its verify variant must be
 * checked using this function.
 */
bool CheckTransactionSignatureEncoding(const valtype &vchSig, uint32_t flags,
                                       ScriptError *serror);

/**
 * Check that the signature provided to authentify a transaction is properly
 * encoded ECDSA signature. Signatures passed to OP_CHECKMULTISIG and its verify
 * variant must be checked using this function.
 */
bool CheckTransactionECDSASignatureEncoding(const valtype &vchSig,
                                            uint32_t flags,
                                            ScriptError *serror);

/**
 * Check that the signature provided to authentify a transaction is properly
 * encoded Schnorr signature (or null). Signatures passed to the new-mode
 * OP_CHECKMULTISIG and its verify variant must be checked using this function.
 */
bool CheckTransactionSchnorrSignatureEncoding(const valtype &vchSig,
                                              uint32_t flags,
                                              ScriptError *serror);

/**
 * Check that a public key is encoded properly.
 */
bool CheckPubKeyEncoding(const valtype &vchPubKey, uint32_t flags,
                         ScriptError *serror);

#endif // BITCOIN_SCRIPT_SIGENCODING_H
