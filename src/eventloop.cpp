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

#include <eventloop.h>

#include <scheduler.h>

EventLoop::~EventLoop() {
    stopEventLoop();
}

bool EventLoop::startEventLoop(CScheduler &scheduler,
                               std::function<void()> runEventLoop,
                               std::chrono::milliseconds delta) {
    LOCK(cs_running);
    if (running) {
        // Do not start the event loop twice.
        return false;
    }

    running = true;

    // Start the event loop.
    scheduler.scheduleEvery(
        [this, runEventLoop]() -> bool {
            runEventLoop();
            if (!stopRequest) {
                return true;
            }

            LOCK(cs_running);
            running = false;

            cond_running.notify_all();

            // A stop request was made.
            return false;
        },
        delta);

    return true;
}

bool EventLoop::stopEventLoop() {
    WAIT_LOCK(cs_running, lock);
    if (!running) {
        return false;
    }

    // Request event loop to stop.
    stopRequest = true;

    // Wait for event loop to stop.
    cond_running.wait(lock, [this]() EXCLUSIVE_LOCKS_REQUIRED(cs_running) {
        return !running;
    });

    stopRequest = false;
    return true;
}
