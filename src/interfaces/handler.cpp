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
// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/handler.h>

#include <util/system.h>

#include <boost/signals2/connection.hpp>
#include <utility>

namespace interfaces {
namespace {

    class HandlerImpl : public Handler {
    public:
        explicit HandlerImpl(boost::signals2::connection connection)
            : m_connection(std::move(connection)) {}

        void disconnect() override { m_connection.disconnect(); }

        boost::signals2::scoped_connection m_connection;
    };

    class CleanupHandler : public Handler {
    public:
        explicit CleanupHandler(std::function<void()> cleanup)
            : m_cleanup(std::move(cleanup)) {}
        ~CleanupHandler() override {
            if (!m_cleanup) {
                return;
            }
            m_cleanup();
            m_cleanup = nullptr;
        }
        void disconnect() override {
            if (!m_cleanup) {
                return;
            }
            m_cleanup();
            m_cleanup = nullptr;
        }
        std::function<void()> m_cleanup;
    };

} // namespace

std::unique_ptr<Handler> MakeHandler(boost::signals2::connection connection) {
    return std::make_unique<HandlerImpl>(std::move(connection));
}

std::unique_ptr<Handler> MakeHandler(std::function<void()> cleanup) {
    return std::make_unique<CleanupHandler>(std::move(cleanup));
}

} // namespace interfaces
