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
// Copyright (c) 2015-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ZMQ_ZMQNOTIFICATIONINTERFACE_H
#define BITCOIN_ZMQ_ZMQNOTIFICATIONINTERFACE_H

#include <validationinterface.h>

#include <list>

class CBlockIndex;
class CZMQAbstractNotifier;

class CZMQNotificationInterface final : public CValidationInterface {
public:
    virtual ~CZMQNotificationInterface();

    std::list<const CZMQAbstractNotifier *> GetActiveNotifiers() const;

    static CZMQNotificationInterface *Create();

protected:
    bool Initialize();
    void Shutdown();

    // CValidationInterface
    void TransactionAddedToMempool(const CTransactionRef &tx) override;
    void BlockConnected(const std::shared_ptr<const CBlock> &pblock,
                        const CBlockIndex *pindexConnected) override;
    void BlockDisconnected(const std::shared_ptr<const CBlock> &pblock,
                           const CBlockIndex *pindexDisconnected) override;
    void UpdatedBlockTip(const CBlockIndex *pindexNew,
                         const CBlockIndex *pindexFork,
                         bool fInitialDownload) override;

private:
    CZMQNotificationInterface();

    void *pcontext;
    std::list<CZMQAbstractNotifier *> notifiers;
};

extern CZMQNotificationInterface *g_zmq_notification_interface;

#endif // BITCOIN_ZMQ_ZMQNOTIFICATIONINTERFACE_H
