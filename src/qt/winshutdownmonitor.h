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
// Copyright (c) 2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_WINSHUTDOWNMONITOR_H
#define BITCOIN_QT_WINSHUTDOWNMONITOR_H

#ifdef WIN32
#include <QByteArray>
#include <QString>

#include <windef.h> // for HWND

#include <QAbstractNativeEventFilter>

class WinShutdownMonitor : public QAbstractNativeEventFilter {
public:
    /** Implements QAbstractNativeEventFilter interface for processing Windows
     * messages */
    bool nativeEventFilter(const QByteArray &eventType, void *pMessage,
                           long *pnResult);

    /** Register the reason for blocking shutdown on Windows to allow clean
     * client exit */
    static void registerShutdownBlockReason(const QString &strReason,
                                            const HWND &mainWinId);
};
#endif

#endif // BITCOIN_QT_WINSHUTDOWNMONITOR_H
