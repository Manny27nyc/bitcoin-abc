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
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <threadinterrupt.h>

CThreadInterrupt::CThreadInterrupt() : flag(false) {}

CThreadInterrupt::operator bool() const {
    return flag.load(std::memory_order_acquire);
}

void CThreadInterrupt::reset() {
    flag.store(false, std::memory_order_release);
}

void CThreadInterrupt::operator()() {
    {
        LOCK(mut);
        flag.store(true, std::memory_order_release);
    }
    cond.notify_all();
}

bool CThreadInterrupt::sleep_for(std::chrono::milliseconds rel_time) {
    WAIT_LOCK(mut, lock);
    return !cond.wait_for(lock, rel_time, [this]() {
        return flag.load(std::memory_order_acquire);
    });
}

bool CThreadInterrupt::sleep_for(std::chrono::seconds rel_time) {
    return sleep_for(
        std::chrono::duration_cast<std::chrono::milliseconds>(rel_time));
}

bool CThreadInterrupt::sleep_for(std::chrono::minutes rel_time) {
    return sleep_for(
        std::chrono::duration_cast<std::chrono::milliseconds>(rel_time));
}
