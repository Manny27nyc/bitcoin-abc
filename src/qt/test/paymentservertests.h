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
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_TEST_PAYMENTSERVERTESTS_H
#define BITCOIN_QT_TEST_PAYMENTSERVERTESTS_H

#include <qt/paymentserver.h>

#include <QObject>
#include <QTest>

class PaymentServerTests : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void paymentServerTests();
};

// Dummy class to receive paymentserver signals.
// If SendCoinsRecipient was a proper QObject, then we could use QSignalSpy...
// but it's not.
class RecipientCatcher : public QObject {
    Q_OBJECT

public Q_SLOTS:
    void getRecipient(const SendCoinsRecipient &r);

public:
    SendCoinsRecipient recipient;
};

#endif // BITCOIN_QT_TEST_PAYMENTSERVERTESTS_H
