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
// Copyright (c) 2017-2019 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/forms/ui_receiverequestdialog.h>
#include <qt/receiverequestdialog.h>

#include <qt/bitcoinunits.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/qrimagewidget.h>
#include <qt/walletmodel.h>

#include <QClipboard>
#include <QPixmap>

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h> /* for USE_QRCODE */
#endif

ReceiveRequestDialog::ReceiveRequestDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::ReceiveRequestDialog), model(nullptr) {
    ui->setupUi(this);

#ifndef USE_QRCODE
    ui->btnSaveAs->setVisible(false);
    ui->lblQRCode->setVisible(false);
#endif

    connect(ui->btnSaveAs, &QPushButton::clicked, ui->lblQRCode,
            &QRImageWidget::saveImage);

    GUIUtil::handleCloseWindowShortcut(this);
}

ReceiveRequestDialog::~ReceiveRequestDialog() {
    delete ui;
}

void ReceiveRequestDialog::setModel(WalletModel *_model) {
    this->model = _model;

    if (_model) {
        connect(_model->getOptionsModel(), &OptionsModel::displayUnitChanged,
                this, &ReceiveRequestDialog::update);
    }

    // update the display unit if necessary
    update();
}

void ReceiveRequestDialog::setInfo(const SendCoinsRecipient &_info) {
    this->info = _info;
    update();
}

void ReceiveRequestDialog::update() {
    if (!model) {
        return;
    }
    QString target = info.label;
    if (target.isEmpty()) {
        target = info.address;
    }
    setWindowTitle(tr("Request payment to %1").arg(target));

    QString uri = GUIUtil::formatBitcoinURI(info);
    ui->btnSaveAs->setEnabled(false);
    QString html;
    html += "<html><font face='verdana, arial, helvetica, sans-serif'>";
    html += "<b>" + tr("Payment information") + "</b><br>";
    html += "<b>" + tr("URI") + "</b>: ";
    html += "<a href=\"" + uri + "\">" + GUIUtil::HtmlEscape(uri) + "</a><br>";
    html += "<b>" + tr("Address") +
            "</b>: " + GUIUtil::HtmlEscape(info.address) + "<br>";
    if (info.amount != Amount::zero()) {
        html += "<b>" + tr("Amount") + "</b>: " +
                BitcoinUnits::formatHtmlWithUnit(
                    model->getOptionsModel()->getDisplayUnit(), info.amount) +
                "<br>";
    }
    if (!info.label.isEmpty()) {
        html += "<b>" + tr("Label") +
                "</b>: " + GUIUtil::HtmlEscape(info.label) + "<br>";
    }
    if (!info.message.isEmpty()) {
        html += "<b>" + tr("Message") +
                "</b>: " + GUIUtil::HtmlEscape(info.message) + "<br>";
    }
    if (model->isMultiwallet()) {
        html += "<b>" + tr("Wallet") +
                "</b>: " + GUIUtil::HtmlEscape(model->getWalletName()) + "<br>";
    }
    ui->outUri->setText(html);

    if (ui->lblQRCode->setQR(uri, info.address)) {
        ui->btnSaveAs->setEnabled(true);
    }
}

void ReceiveRequestDialog::on_btnCopyURI_clicked() {
    GUIUtil::setClipboard(GUIUtil::formatBitcoinURI(info));
}

void ReceiveRequestDialog::on_btnCopyAddress_clicked() {
    GUIUtil::setClipboard(info.address);
}
