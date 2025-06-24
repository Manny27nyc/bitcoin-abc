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
/***********************************************************************
 * Copyright (c) 2017 Amaury SÉCHET                                    *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_MODULE_SCHNORR_H
#define SECP256K1_MODULE_SCHNORR_H

#include "scalar.h"
#include "group.h"

static int secp256k1_schnorr_sig_verify(
    const secp256k1_ecmult_context* ctx,
    const unsigned char *sig64,
    secp256k1_ge *pubkey,
    const unsigned char *msg32
);

static int secp256k1_schnorr_compute_e(
    secp256k1_scalar* res,
    const unsigned char *r,
    secp256k1_ge *pubkey,
    const unsigned char *msg32
);

static int secp256k1_schnorr_sig_sign(
    const secp256k1_context* ctx,
    unsigned char *sig64,
    const unsigned char *msg32,
    const secp256k1_scalar *privkey,
    secp256k1_ge *pubkey,
    secp256k1_nonce_function noncefp,
    const void *ndata
);

static int secp256k1_schnorr_sig_generate_k(
    const secp256k1_context* ctx,
    secp256k1_scalar *k,
    const unsigned char *msg32,
    const secp256k1_scalar *privkey,
    secp256k1_nonce_function noncefp,
    const void *ndata
);

#endif
