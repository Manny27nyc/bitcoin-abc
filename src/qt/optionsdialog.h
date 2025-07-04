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

#ifndef BITCOIN_QT_OPTIONSDIALOG_H
#define BITCOIN_QT_OPTIONSDIALOG_H

#include <QDialog>
#include <QValidator>

class OptionsModel;
class QValidatedLineEdit;

QT_BEGIN_NAMESPACE
class QDataWidgetMapper;
QT_END_NAMESPACE

namespace Ui {
class OptionsDialog;
}

/** Proxy address widget validator, checks for a valid proxy address. */
class ProxyAddressValidator : public QValidator {
    Q_OBJECT

public:
    explicit ProxyAddressValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

/** Third party tx URL validator, checks for an https link. */
class ThirdPartyTxUrlsValidator : public QValidator {
    Q_OBJECT

public:
    explicit ThirdPartyTxUrlsValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

/** Preferences dialog. */
class OptionsDialog : public QDialog {
    Q_OBJECT

public:
    explicit OptionsDialog(QWidget *parent, bool enableWallet);
    ~OptionsDialog();

    enum Tab {
        TAB_MAIN,
        TAB_NETWORK,
    };

    void setModel(OptionsModel *model);
    void setMapper();
    void setCurrentTab(OptionsDialog::Tab tab);

private Q_SLOTS:
    /* set OK button state (enabled / disabled) */
    void setOkButtonState(bool fState);
    void on_resetButton_clicked();
    void on_openBitcoinConfButton_clicked();
    void on_okButton_clicked();
    void on_cancelButton_clicked();

    void on_hideTrayIcon_stateChanged(int fState);

    void togglePruneWarning(bool enabled);
    void showRestartWarning(bool fPersistent = false);
    void clearStatusLabel();
    void updateProxyValidationState();
    /* query the networks, for which the default proxy is used */
    void updateDefaultProxyNets();
    void updateThirdPartyTxUrlsState();

Q_SIGNALS:
    void proxyIpChecks(QValidatedLineEdit *pUiProxyIp, int nProxyPort);

private:
    Ui::OptionsDialog *ui;
    OptionsModel *model;
    QDataWidgetMapper *mapper;
};

#endif // BITCOIN_QT_OPTIONSDIALOG_H
