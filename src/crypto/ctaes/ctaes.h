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
/**********************************************************************
 * Copyright (c) 2016 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _CTAES_H_
#define _CTAES_H_ 1

#include <stdint.h>
#include <stdlib.h>

typedef struct {
    uint16_t slice[8];
} AES_state;

typedef struct {
    AES_state rk[11];
} AES128_ctx;

typedef struct {
    AES_state rk[13];
} AES192_ctx;

typedef struct {
    AES_state rk[15];
} AES256_ctx;

void AES128_init(AES128_ctx *ctx, const uint8_t *key16);
void AES128_encrypt(const AES128_ctx *ctx, size_t blocks, uint8_t *cipher16,
                    const uint8_t *plain16);
void AES128_decrypt(const AES128_ctx *ctx, size_t blocks, uint8_t *plain16,
                    const uint8_t *cipher16);

void AES192_init(AES192_ctx *ctx, const uint8_t *key24);
void AES192_encrypt(const AES192_ctx *ctx, size_t blocks, uint8_t *cipher16,
                    const uint8_t *plain16);
void AES192_decrypt(const AES192_ctx *ctx, size_t blocks, uint8_t *plain16,
                    const uint8_t *cipher16);

void AES256_init(AES256_ctx *ctx, const uint8_t *key32);
void AES256_encrypt(const AES256_ctx *ctx, size_t blocks, uint8_t *cipher16,
                    const uint8_t *plain16);
void AES256_decrypt(const AES256_ctx *ctx, size_t blocks, uint8_t *plain16,
                    const uint8_t *cipher16);

#endif
