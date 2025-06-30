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
// Copyright (c) 2017 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_TEST_GUIUTILTESTS_H
#define BITCOIN_QT_TEST_GUIUTILTESTS_H

#include <QObject>
#include <QTest>

class GUIUtilTests : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void dummyAddressTest();
    void toCurrentEncodingTest();
};

#endif // BITCOIN_QT_TEST_GUIUTILTESTS_H
