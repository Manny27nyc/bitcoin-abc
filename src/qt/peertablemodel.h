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

#ifndef BITCOIN_QT_PEERTABLEMODEL_H
#define BITCOIN_QT_PEERTABLEMODEL_H

#include <net.h>
#include <net_processing.h> // For CNodeStateStats

#include <QAbstractTableModel>
#include <QStringList>

#include <memory>

class PeerTablePriv;

namespace interfaces {
class Node;
}

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

struct CNodeCombinedStats {
    CNodeStats nodeStats;
    CNodeStateStats nodeStateStats;
    bool fNodeStateStatsAvailable;
};

class NodeLessThan {
public:
    NodeLessThan(int nColumn, Qt::SortOrder fOrder)
        : column(nColumn), order(fOrder) {}
    bool operator()(const CNodeCombinedStats &left,
                    const CNodeCombinedStats &right) const;

private:
    int column;
    Qt::SortOrder order;
};

/**
 * Qt model providing information about connected peers, similar to the
 * "getpeerinfo" RPC call. Used by the rpc console UI.
 */
class PeerTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit PeerTableModel(interfaces::Node &node, QObject *parent);
    ~PeerTableModel();
    const CNodeCombinedStats *getNodeStats(int idx);
    int getRowByNodeId(NodeId nodeid);
    void startAutoRefresh();
    void stopAutoRefresh();

    enum ColumnIndex {
        NetNodeId = 0,
        Address = 1,
        Ping = 2,
        Sent = 3,
        Received = 4,
        Subversion = 5,
    };

    /** @name Methods overridden from QAbstractTableModel
        @{*/
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    void sort(int column, Qt::SortOrder order) override;
    /*@}*/

public Q_SLOTS:
    void refresh();

private:
    interfaces::Node &m_node;
    QStringList columns;
    std::unique_ptr<PeerTablePriv> priv;
    QTimer *timer;
};

#endif // BITCOIN_QT_PEERTABLEMODEL_H
