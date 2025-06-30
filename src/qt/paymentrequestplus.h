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

#ifndef BITCOIN_QT_PAYMENTREQUESTPLUS_H
#define BITCOIN_QT_PAYMENTREQUESTPLUS_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <qt/paymentrequest.pb.h>
#pragma GCC diagnostic pop

#include <amount.h>
#include <script/script.h>

#include <openssl/x509.h>

#include <QByteArray>
#include <QList>
#include <QString>

static const bool DEFAULT_SELFSIGNED_ROOTCERTS = false;

//
// Wraps dumb protocol buffer paymentRequest with extra methods
//

class PaymentRequestPlus {
public:
    PaymentRequestPlus() {}

    bool parse(const QByteArray &data);
    bool SerializeToString(std::string *output) const;

    bool IsInitialized() const;
    // Returns true if merchant's identity is authenticated, and returns
    // human-readable merchant identity in merchant
    bool getMerchant(X509_STORE *certStore, QString &merchant) const;

    // Returns list of outputs, amount
    QList<std::pair<CScript, Amount>> getPayTo() const;

    const payments::PaymentDetails &getDetails() const { return details; }

private:
    payments::PaymentRequest paymentRequest;
    payments::PaymentDetails details;
};

#endif // BITCOIN_QT_PAYMENTREQUESTPLUS_H
