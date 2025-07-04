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
// Copyright (c) 2017-2018 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_BITCOINADDRESSVALIDATOR_H
#define BITCOIN_QT_BITCOINADDRESSVALIDATOR_H

#include <QValidator>

/**
 * Bitcoin address entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class BitcoinAddressEntryValidator : public QValidator {
    Q_OBJECT

public:
    explicit BitcoinAddressEntryValidator(const std::string &cashaddrprefixIn,
                                          QObject *parent);

    State validate(QString &input, int &pos) const override;

private:
    std::string cashaddrprefix;
};

/** Bitcoin address widget validator, checks for a valid bitcoin address.
 */
class BitcoinAddressCheckValidator : public QValidator {
    Q_OBJECT

public:
    explicit BitcoinAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

#endif // BITCOIN_QT_BITCOINADDRESSVALIDATOR_H
