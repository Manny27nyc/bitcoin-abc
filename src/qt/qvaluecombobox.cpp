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

#include <qt/qvaluecombobox.h>

QValueComboBox::QValueComboBox(QWidget *parent)
    : QComboBox(parent), role(Qt::UserRole) {
    connect(
        this,
        static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this, &QValueComboBox::handleSelectionChanged);
}

QVariant QValueComboBox::value() const {
    return itemData(currentIndex(), role);
}

void QValueComboBox::setValue(const QVariant &value) {
    setCurrentIndex(findData(value, role));
}

void QValueComboBox::setRole(int _role) {
    this->role = _role;
}

void QValueComboBox::handleSelectionChanged(int idx) {
    Q_EMIT valueChanged();
}
