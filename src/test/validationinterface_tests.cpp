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
// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <consensus/validation.h>
#include <primitives/block.h>
#include <scheduler.h>
#include <test/util/setup_common.h>
#include <util/check.h>
#include <validationinterface.h>

BOOST_FIXTURE_TEST_SUITE(validationinterface_tests, TestingSetup)

struct TestSubscriberNoop final : public CValidationInterface {
    void BlockChecked(const CBlock &, const BlockValidationState &) override {}
};

BOOST_AUTO_TEST_CASE(unregister_validation_interface_race) {
    std::atomic<bool> generate{true};

    // Start thread to generate notifications
    std::thread gen{[&] {
        const CBlock block_dummy;
        BlockValidationState state_dummy;
        while (generate) {
            GetMainSignals().BlockChecked(block_dummy, state_dummy);
        }
    }};

    // Start thread to consume notifications
    std::thread sub{[&] {
        // keep going for about 1 sec, which is 250k iterations
        for (int i = 0; i < 250000; i++) {
            auto subscriber = std::make_shared<TestSubscriberNoop>();
            RegisterSharedValidationInterface(subscriber);
            UnregisterSharedValidationInterface(subscriber);
        }
        // tell the other thread we are done
        generate = false;
    }};

    gen.join();
    sub.join();
    BOOST_CHECK(!generate);
}

class TestInterface : public CValidationInterface {
public:
    TestInterface(std::function<void()> on_call = nullptr,
                  std::function<void()> on_destroy = nullptr)
        : m_on_call(std::move(on_call)), m_on_destroy(std::move(on_destroy)) {}
    virtual ~TestInterface() {
        if (m_on_destroy) {
            m_on_destroy();
        }
    }
    void BlockChecked(const CBlock &block,
                      const BlockValidationState &state) override {
        if (m_on_call) {
            m_on_call();
        }
    }
    static void Call() {
        CBlock block;
        BlockValidationState state;
        GetMainSignals().BlockChecked(block, state);
    }
    std::function<void()> m_on_call;
    std::function<void()> m_on_destroy;
};

// Regression test to ensure UnregisterAllValidationInterfaces calls don't
// destroy a validation interface while it is being called. Bug:
// https://github.com/bitcoin/bitcoin/pull/18551
BOOST_AUTO_TEST_CASE(unregister_all_during_call) {
    bool destroyed = false;
    RegisterSharedValidationInterface(std::make_shared<TestInterface>(
        [&] {
            // First call should decrements reference count 2 -> 1
            UnregisterAllValidationInterfaces();
            BOOST_CHECK(!destroyed);
            // Second call should not decrement reference count 1 -> 0
            UnregisterAllValidationInterfaces();
            BOOST_CHECK(!destroyed);
        },
        [&] { destroyed = true; }));
    TestInterface::Call();
    BOOST_CHECK(destroyed);
}

BOOST_AUTO_TEST_SUITE_END()
