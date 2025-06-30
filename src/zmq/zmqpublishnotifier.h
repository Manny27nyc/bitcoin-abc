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
// Copyright (c) 2015-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ZMQ_ZMQPUBLISHNOTIFIER_H
#define BITCOIN_ZMQ_ZMQPUBLISHNOTIFIER_H

#include <zmq/zmqabstractnotifier.h>

class CBlockIndex;

class CZMQAbstractPublishNotifier : public CZMQAbstractNotifier {
private:
    //! upcounting per message sequence number
    uint32_t nSequence{0U};

public:
    /* send zmq multipart message
       parts:
          * command
          * data
          * message sequence number
    */
    bool SendMessage(const char *command, const void *data, size_t size);

    bool Initialize(void *pcontext) override;
    void Shutdown() override;
};

class CZMQPublishHashBlockNotifier : public CZMQAbstractPublishNotifier {
public:
    bool NotifyBlock(const CBlockIndex *pindex) override;
};

class CZMQPublishHashTransactionNotifier : public CZMQAbstractPublishNotifier {
public:
    bool NotifyTransaction(const CTransaction &transaction) override;
};

class CZMQPublishRawBlockNotifier : public CZMQAbstractPublishNotifier {
public:
    bool NotifyBlock(const CBlockIndex *pindex) override;
};

class CZMQPublishRawTransactionNotifier : public CZMQAbstractPublishNotifier {
public:
    bool NotifyTransaction(const CTransaction &transaction) override;
};

#endif // BITCOIN_ZMQ_ZMQPUBLISHNOTIFIER_H
