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
#ifndef _SECP256K1_SCHNORR_
# define _SECP256K1_SCHNORR_

# include "secp256k1.h"

# ifdef __cplusplus
extern "C" {
# endif

/**
 * Verify a signature created by secp256k1_schnorr_sign.
 * Returns: 1: correct signature
 *          0: incorrect signature
 * Args:    ctx:       a secp256k1 context object, initialized for verification.
 * In:      sig64:     the 64-byte signature being verified (cannot be NULL)
 *          msghash32: the 32-byte message hash being verified (cannot be NULL).
 *                     The verifier must make sure to apply a cryptographic
 *                     hash function to the message by itself and not accept an
 *                     msghash32 value directly. Otherwise, it would be easy to
 *                     create a "valid" signature without knowledge of the
 *                     secret key.
 *          pubkey:    the public key to verify with (cannot be NULL)
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_schnorr_verify(
  const secp256k1_context* ctx,
  const unsigned char *sig64,
  const unsigned char *msghash32,
  const secp256k1_pubkey *pubkey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/**
 * Create a signature using a custom EC-Schnorr-SHA256 construction. It
 * produces non-malleable 64-byte signatures which support batch validation,
 * and multiparty signing.
 * Returns: 1: signature created
 *          0: the nonce generation function failed, or the private key was
 *             invalid.
 * Args:    ctx:       pointer to a context object, initialized for signing
 *                     (cannot be NULL)
 * Out:     sig64:     pointer to a 64-byte array where the signature will be
 *                     placed (cannot be NULL)
 * In:      msghash32: the 32-byte message hash being signed (cannot be NULL).
 *          seckey:    pointer to a 32-byte secret key (cannot be NULL)
 *          noncefp:   pointer to a nonce generation function. If NULL,
 *                     secp256k1_nonce_function_default is used
 *          ndata:     pointer to arbitrary data used by the nonce generation
 *                     function (can be NULL)
 */
SECP256K1_API int secp256k1_schnorr_sign(
  const secp256k1_context *ctx,
  unsigned char *sig64,
  const unsigned char *msghash32,
  const unsigned char *seckey,
  secp256k1_nonce_function noncefp,
  const void *ndata
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

# ifdef __cplusplus
}
# endif

#endif
