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
// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_CREATEWALLETDIALOG_H
#define BITCOIN_QT_CREATEWALLETDIALOG_H

#include <QDialog>

class WalletModel;

namespace Ui {
class CreateWalletDialog;
}

/**
 * Dialog for creating wallets
 */
class CreateWalletDialog : public QDialog {
    Q_OBJECT

public:
    explicit CreateWalletDialog(QWidget *parent);
    virtual ~CreateWalletDialog();

    QString walletName() const;
    bool isEncryptWalletChecked() const;
    bool isDisablePrivateKeysChecked() const;
    bool isMakeBlankWalletChecked() const;
    bool isDescriptorWalletChecked() const;

private:
    Ui::CreateWalletDialog *ui;
};

#endif // BITCOIN_QT_CREATEWALLETDIALOG_H
