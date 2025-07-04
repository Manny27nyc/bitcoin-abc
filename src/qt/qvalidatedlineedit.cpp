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

#include <qt/qvalidatedlineedit.h>

#include <qt/bitcoinaddressvalidator.h>
#include <qt/guiconstants.h>

QValidatedLineEdit::QValidatedLineEdit(QWidget *parent)
    : QLineEdit(parent), valid(true), checkValidator(nullptr) {
    connect(this, &QValidatedLineEdit::textChanged, this,
            &QValidatedLineEdit::markValid);
}

void QValidatedLineEdit::setValid(bool _valid) {
    if (_valid == this->valid) {
        return;
    }

    if (_valid) {
        setStyleSheet("");
    } else {
        setStyleSheet(STYLE_INVALID);
    }
    this->valid = _valid;

    Q_EMIT validationDidChange(this);
}

void QValidatedLineEdit::focusInEvent(QFocusEvent *evt) {
    // Clear invalid flag on focus
    setValid(true);

    QLineEdit::focusInEvent(evt);
}

void QValidatedLineEdit::focusOutEvent(QFocusEvent *evt) {
    checkValidity();

    QLineEdit::focusOutEvent(evt);
}

void QValidatedLineEdit::markValid() {
    // As long as a user is typing ensure we display state as valid
    setValid(true);
}

void QValidatedLineEdit::clear() {
    setValid(true);
    QLineEdit::clear();
}

void QValidatedLineEdit::setEnabled(bool enabled) {
    if (!enabled) {
        // A disabled QValidatedLineEdit should be marked valid
        setValid(true);
    } else {
        // Recheck validity when QValidatedLineEdit gets enabled
        checkValidity();
    }

    QLineEdit::setEnabled(enabled);
}

void QValidatedLineEdit::checkValidity() {
    if (text().isEmpty()) {
        setValid(true);
    } else if (hasAcceptableInput()) {
        setValid(true);

        // Check contents on focus out
        if (checkValidator) {
            QString address = text();
            int pos = 0;
            if (checkValidator->validate(address, pos) ==
                QValidator::Acceptable) {
                setValid(true);
            } else {
                setValid(false);
            }
        }
    } else {
        setValid(false);
    }
}

void QValidatedLineEdit::setCheckValidator(const QValidator *v) {
    checkValidator = v;
}

bool QValidatedLineEdit::isValid() {
    // use checkValidator in case the QValidatedLineEdit is disabled
    if (checkValidator) {
        QString address = text();
        int pos = 0;
        if (checkValidator->validate(address, pos) == QValidator::Acceptable) {
            return true;
        }
    }

    return valid;
}
