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

#include <qt/forms/ui_transactiondescdialog.h>
#include <qt/transactiondescdialog.h>

#include <qt/guiutil.h>
#include <qt/transactiontablemodel.h>

#include <QModelIndex>

TransactionDescDialog::TransactionDescDialog(const QModelIndex &idx,
                                             QWidget *parent)
    : QDialog(parent), ui(new Ui::TransactionDescDialog) {
    ui->setupUi(this);
    setWindowTitle(
        tr("Details for %1")
            .arg(idx.data(TransactionTableModel::TxIDRole).toString()));
    QString desc =
        idx.data(TransactionTableModel::LongDescriptionRole).toString();
    ui->detailText->setHtml(desc);

    GUIUtil::handleCloseWindowShortcut(this);
}

TransactionDescDialog::~TransactionDescDialog() {
    delete ui;
}
