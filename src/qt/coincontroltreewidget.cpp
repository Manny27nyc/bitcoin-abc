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
// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/coincontroltreewidget.h>

#include <qt/coincontroldialog.h>

CoinControlTreeWidget::CoinControlTreeWidget(QWidget *parent)
    : QTreeWidget(parent) {}

void CoinControlTreeWidget::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space) // press spacebar -> select checkbox
    {
        event->ignore();
        if (this->currentItem()) {
            int COLUMN_CHECKBOX = 0;
            this->currentItem()->setCheckState(
                COLUMN_CHECKBOX, ((this->currentItem()->checkState(
                                       COLUMN_CHECKBOX) == Qt::Checked)
                                      ? Qt::Unchecked
                                      : Qt::Checked));
        }
    } else if (event->key() == Qt::Key_Escape) // press esc -> close dialog
    {
        event->ignore();
        CoinControlDialog *coinControlDialog =
            static_cast<CoinControlDialog *>(this->parentWidget());
        coinControlDialog->done(QDialog::Accepted);
    } else {
        this->QTreeWidget::keyPressEvent(event);
    }
}
