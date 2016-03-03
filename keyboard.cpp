#include "keyboard.h"
#include "ui_keyboard.h"

keyboard::keyboard(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::keyboard)
{
    ui->setupUi(this);
}

keyboard::~keyboard()
{
    delete ui;
}

QString keyboard::returnValue()
{
    exec();
    return inputbuff;
}

void keyboard::on_pushButtoncon_clicked()
{

}
