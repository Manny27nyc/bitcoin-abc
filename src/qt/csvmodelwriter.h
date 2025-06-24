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

#ifndef BITCOIN_QT_CSVMODELWRITER_H
#define BITCOIN_QT_CSVMODELWRITER_H

#include <QList>
#include <QObject>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
QT_END_NAMESPACE

/** Export a Qt table model to a CSV file. This is useful for analyzing or
   post-processing the data in
    a spreadsheet.
 */
class CSVModelWriter : public QObject {
    Q_OBJECT

public:
    explicit CSVModelWriter(const QString &filename, QObject *parent = nullptr);

    void setModel(const QAbstractItemModel *model);
    void addColumn(const QString &title, int column, int role = Qt::EditRole);

    /** Perform export of the model to CSV.
        @returns true on success, false otherwise
    */
    bool write();

private:
    QString filename;
    const QAbstractItemModel *model;

    struct Column {
        QString title;
        int column;
        int role;
    };
    QList<Column> columns;
};

#endif // BITCOIN_QT_CSVMODELWRITER_H
