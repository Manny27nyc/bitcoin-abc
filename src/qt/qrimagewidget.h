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
// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_QRIMAGEWIDGET_H
#define BITCOIN_QT_QRIMAGEWIDGET_H

#include <QImage>
#include <QLabel>

/* Maximum allowed URI length */
static const int MAX_URI_LENGTH = 255;

/* Size of exported QR Code image */
static const int QR_IMAGE_SIZE = 350;

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

/**
 * Label widget for QR code. This image can be dragged, dropped, copied and
 * saved to disk.
 */
class QRImageWidget : public QLabel {
    Q_OBJECT

public:
    explicit QRImageWidget(QWidget *parent = nullptr);
    bool hasPixmap() const;
    bool setQR(const QString &qrData, const QString &text = "");
    QImage exportImage();

public Q_SLOTS:
    void saveImage();
    void copyImage();

protected:
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void contextMenuEvent(QContextMenuEvent *event) override;

private:
    QMenu *contextMenu;
};

#endif // BITCOIN_QT_QRIMAGEWIDGET_H
