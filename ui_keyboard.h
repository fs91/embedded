/********************************************************************************
** Form generated from reading UI file 'keyboard.ui'
**
** Created: Sun Jun 23 20:33:13 2013
**      by: Qt User Interface Compiler version 4.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_KEYBOARD_H
#define UI_KEYBOARD_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_keyboard
{
public:

    void setupUi(QWidget *keyboard)
    {
        if (keyboard->objectName().isEmpty())
            keyboard->setObjectName(QString::fromUtf8("keyboard"));
        keyboard->resize(240, 320);

        retranslateUi(keyboard);

        QMetaObject::connectSlotsByName(keyboard);
    } // setupUi

    void retranslateUi(QWidget *keyboard)
    {
        keyboard->setWindowTitle(QApplication::translate("keyboard", "Form", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class keyboard: public Ui_keyboard {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_KEYBOARD_H
