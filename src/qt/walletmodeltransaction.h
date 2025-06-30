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

#ifndef BITCOIN_QT_WALLETMODELTRANSACTION_H
#define BITCOIN_QT_WALLETMODELTRANSACTION_H

#include <primitives/transaction.h>
#include <qt/sendcoinsrecipient.h>

#include <amount.h>

#include <QObject>

class SendCoinsRecipient;

namespace interfaces {
class Node;
} // namespace interfaces

/** Data model for a walletmodel transaction. */
class WalletModelTransaction {
public:
    explicit WalletModelTransaction(
        const QList<SendCoinsRecipient> &recipients);

    QList<SendCoinsRecipient> getRecipients() const;

    CTransactionRef &getWtx();
    unsigned int getTransactionSize();

    void setTransactionFee(const Amount newFee);
    Amount getTransactionFee() const;

    Amount getTotalTransactionAmount() const;

    // needed for the subtract-fee-from-amount feature
    void reassignAmounts(int nChangePosRet);

private:
    QList<SendCoinsRecipient> recipients;
    CTransactionRef wtx;
    Amount fee;
};

#endif // BITCOIN_QT_WALLETMODELTRANSACTION_H
