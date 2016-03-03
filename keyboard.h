#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <QWidget>

namespace Ui {
    class keyboard;
}

class keyboard : public QWidget
{
    Q_OBJECT

public:
    explicit keyboard(QWidget *parent = 0);
    bool filterEvent(const QEvent* event);

    QString identifierName();
    QString language();

    bool isComposing() const;

    void reset;

    ~keyboard();

private:
    Ui::keyboard *ui;

private slots:
    void sendCharacter(QChar character);
};

#endif // KEYBOARD_H
