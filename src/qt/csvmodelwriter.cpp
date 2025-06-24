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

#include <qt/csvmodelwriter.h>

#include <QAbstractItemModel>
#include <QFile>
#include <QTextStream>

CSVModelWriter::CSVModelWriter(const QString &_filename, QObject *parent)
    : QObject(parent), filename(_filename), model(nullptr) {}

void CSVModelWriter::setModel(const QAbstractItemModel *_model) {
    this->model = _model;
}

void CSVModelWriter::addColumn(const QString &title, int column, int role) {
    Column col;
    col.title = title;
    col.column = column;
    col.role = role;

    columns.append(col);
}

static void writeValue(QTextStream &f, const QString &value) {
    QString escaped = value;
    escaped.replace('"', "\"\"");
    f << "\"" << escaped << "\"";
}

static void writeSep(QTextStream &f) {
    f << ",";
}

static void writeNewline(QTextStream &f) {
    f << "\n";
}

bool CSVModelWriter::write() {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    QTextStream out(&file);

    int numRows = 0;
    if (model) {
        numRows = model->rowCount();
    }

    // Header row
    for (int i = 0; i < columns.size(); ++i) {
        if (i != 0) {
            writeSep(out);
        }
        writeValue(out, columns[i].title);
    }
    writeNewline(out);

    // Data rows
    for (int j = 0; j < numRows; ++j) {
        for (int i = 0; i < columns.size(); ++i) {
            if (i != 0) {
                writeSep(out);
            }
            QVariant data =
                model->index(j, columns[i].column).data(columns[i].role);
            writeValue(out, data.toString());
        }
        writeNewline(out);
    }

    file.close();

    return file.error() == QFile::NoError;
}
