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
// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/forms/ui_openuridialog.h>
#include <qt/openuridialog.h>

#include <chainparams.h>
#include <qt/guiutil.h>
#include <qt/sendcoinsrecipient.h>

#include <QUrl>

OpenURIDialog::OpenURIDialog(const CChainParams &params, QWidget *parent)
    : QDialog(parent), ui(new Ui::OpenURIDialog),
      uriScheme(QString::fromStdString(params.CashAddrPrefix())) {
    ui->setupUi(this);
    ui->uriEdit->setPlaceholderText(uriScheme + ":");

    GUIUtil::handleCloseWindowShortcut(this);
}

OpenURIDialog::~OpenURIDialog() {
    delete ui;
}

QString OpenURIDialog::getURI() {
    return ui->uriEdit->text();
}

void OpenURIDialog::accept() {
    SendCoinsRecipient rcp;
    if (GUIUtil::parseBitcoinURI(uriScheme, getURI(), &rcp)) {
        /* Only accept value URIs */
        QDialog::accept();
    } else {
        ui->uriEdit->setValid(false);
    }
}

void OpenURIDialog::on_selectFileButton_clicked() {
    QString filename = GUIUtil::getOpenFileName(
        this, tr("Select payment request file to open"), "", "", nullptr);
    if (filename.isEmpty()) {
        return;
    }
    QUrl fileUri = QUrl::fromLocalFile(filename);
    ui->uriEdit->setText(uriScheme +
                         ":?r=" + QUrl::toPercentEncoding(fileUri.toString()));
}
