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
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <support/cleanse.h>

#include <cstring>

#if defined(_MSC_VER)
#include <Windows.h> // For SecureZeroMemory.
#endif

void memory_cleanse(void *ptr, size_t len) {
#if defined(_MSC_VER)
    /* SecureZeroMemory is guaranteed not to be optimized out by MSVC. */
    SecureZeroMemory(ptr, len);
#else
    std::memset(ptr, 0, len);

    /*
     * Memory barrier that scares the compiler away from optimizing out the
     * memset.
     *
     * Quoting Adam Langley <agl@google.com> in commit
     * ad1907fe73334d6c696c8539646c21b11178f20f in BoringSSL (ISC License):
     *   As best as we can tell, this is sufficient to break any optimisations
     *   that might try to eliminate "superfluous" memsets.
     * This method is used in memzero_explicit() the Linux kernel, too. Its
     * advantage is that it is pretty efficient because the compiler can still
     * implement the memset() efficiently, just not remove it entirely. See
     * "Dead Store Elimination (Still) Considered Harmful" by Yang et al.
     * (USENIX Security 2017) for more background.
     */
    __asm__ __volatile__("" : : "r"(ptr) : "memory");
#endif
}
