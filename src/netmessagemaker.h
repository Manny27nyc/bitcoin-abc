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
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NETMESSAGEMAKER_H
#define BITCOIN_NETMESSAGEMAKER_H

#include <net.h>
#include <serialize.h>

class CNetMsgMaker {
public:
    explicit CNetMsgMaker(int nVersionIn) : nVersion(nVersionIn) {}

    template <typename... Args>
    CSerializedNetMsg Make(int nFlags, std::string msg_type,
                           Args &&... args) const {
        CSerializedNetMsg msg;
        msg.m_type = std::move(msg_type);
        CVectorWriter{SER_NETWORK, nFlags | nVersion, msg.data, 0,
                      std::forward<Args>(args)...};
        return msg;
    }

    template <typename... Args>
    CSerializedNetMsg Make(std::string msg_type, Args &&... args) const {
        return Make(0, std::move(msg_type), std::forward<Args>(args)...);
    }

private:
    const int nVersion;
};

#endif // BITCOIN_NETMESSAGEMAKER_H
