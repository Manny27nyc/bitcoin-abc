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
// Copyright (c) 2017 The Bitcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/bitcoinaddressvalidator.h>
#include <qt/test/bitcoinaddressvalidatortests.h>

#include <chainparams.h>

#include <QValidator>

void BitcoinAddressValidatorTests::inputTests() {
    const auto params = CreateChainParams(CBaseChainParams::MAIN);
    const std::string prefix = params->CashAddrPrefix();
    BitcoinAddressEntryValidator v(prefix, nullptr);

    int unused = 0;
    QString in;

    // Empty string is intermediate.
    in = "";
    QVERIFY(v.validate(in, unused) == QValidator::Intermediate);

    // invalid base58 because of I, invalid cashaddr, currently considered valid
    // anyway.
    in = "ICASH";
    QVERIFY(v.validate(in, unused) == QValidator::Acceptable);

    // invalid base58, invalid cashaddr, currently considered valid anyway.
    in = "EOASH";
    QVERIFY(v.validate(in, unused) == QValidator::Acceptable);

    // invalid base58 because of I, but could be a cashaddr prefix
    in = "ECASI";
    QVERIFY(v.validate(in, unused) == QValidator::Acceptable);

    // invalid base58, valid cashaddr
    in = "ECASH:OP";
    QVERIFY(v.validate(in, unused) == QValidator::Acceptable);

    // invalid base58, valid cashaddr, lower case
    in = "ecash:op";
    QVERIFY(v.validate(in, unused) == QValidator::Acceptable);

    // invalid base58, valid cashaddr, mixed case
    in = "eCash:Op";
    QVERIFY(v.validate(in, unused) == QValidator::Acceptable);

    // valid base58, invalid cash
    in = "EEEEEEEEEEEEEE";
    QVERIFY(v.validate(in, unused) == QValidator::Acceptable);

    // Only alphanumeric chars are accepted.
    in = "%";
    QVERIFY(v.validate(in, unused) == QValidator::Invalid);
}
