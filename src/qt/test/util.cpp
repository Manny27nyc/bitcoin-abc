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
#include <qt/test/util.h>

#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QString>
#include <QTimer>
#include <QWidget>

void ConfirmMessage(QString *text, int msec) {
    QTimer::singleShot(msec, [text] {
        for (QWidget *widget : QApplication::topLevelWidgets()) {
            if (widget->inherits("QMessageBox")) {
                QMessageBox *messageBox = qobject_cast<QMessageBox *>(widget);
                if (text) {
                    *text = messageBox->text();
                }
                messageBox->defaultButton()->click();
            }
        }
    });
}
