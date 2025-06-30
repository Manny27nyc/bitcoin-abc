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
/***********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                              *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_ECKEY_H
#define SECP256K1_ECKEY_H

#include <stddef.h>

#include "group.h"
#include "scalar.h"
#include "ecmult.h"
#include "ecmult_gen.h"

static int secp256k1_eckey_pubkey_parse(secp256k1_ge *elem, const unsigned char *pub, size_t size);
static int secp256k1_eckey_pubkey_serialize(secp256k1_ge *elem, unsigned char *pub, size_t *size, int compressed);

static int secp256k1_eckey_privkey_tweak_add(secp256k1_scalar *key, const secp256k1_scalar *tweak);
static int secp256k1_eckey_pubkey_tweak_add(const secp256k1_ecmult_context *ctx, secp256k1_ge *key, const secp256k1_scalar *tweak);
static int secp256k1_eckey_privkey_tweak_mul(secp256k1_scalar *key, const secp256k1_scalar *tweak);
static int secp256k1_eckey_pubkey_tweak_mul(const secp256k1_ecmult_context *ctx, secp256k1_ge *key, const secp256k1_scalar *tweak);

#endif /* SECP256K1_ECKEY_H */
