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
// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_SPLASHSCREEN_H
#define BITCOIN_QT_SPLASHSCREEN_H

#include <QWidget>

#include <memory>

class NetworkStyle;

namespace interfaces {
class Handler;
class Node;
class Wallet;
}; // namespace interfaces

/** Class for the splashscreen with information of the running client.
 *
 * @note this is intentionally not a QSplashScreen. Bitcoin Core initialization
 * can take a long time, and in that case a progress window that cannot be moved
 * around and minimized has turned out to be frustrating to the user.
 */
class SplashScreen : public QWidget {
    Q_OBJECT

public:
    explicit SplashScreen(const NetworkStyle *networkStyle);
    ~SplashScreen();
    void setNode(interfaces::Node &node);

protected:
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

public Q_SLOTS:
    /** Hide the splash screen window and schedule the splash screen object for
     * deletion */
    void finish();

    /** Show message and progress */
    void showMessage(const QString &message, int alignment,
                     const QColor &color);

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    /** Connect core signals to splash screen */
    void subscribeToCoreSignals();
    /** Disconnect core signals to splash screen */
    void unsubscribeFromCoreSignals();
    /** Initiate shutdown */
    void shutdown();
    /** Connect wallet signals to splash screen */
    void ConnectWallet(std::unique_ptr<interfaces::Wallet> wallet);

    QPixmap pixmap;
    QString curMessage;
    QColor curColor;
    int curAlignment;

    interfaces::Node *m_node = nullptr;
    bool m_shutdown = false;
    std::unique_ptr<interfaces::Handler> m_handler_init_message;
    std::unique_ptr<interfaces::Handler> m_handler_show_progress;
    std::unique_ptr<interfaces::Handler> m_handler_load_wallet;
    std::list<std::unique_ptr<interfaces::Wallet>> m_connected_wallets;
    std::list<std::unique_ptr<interfaces::Handler>> m_connected_wallet_handlers;
};

#endif // BITCOIN_QT_SPLASHSCREEN_H
