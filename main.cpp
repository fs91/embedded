#include <QtGui/QApplication>
#include "myinputpanelcontext.h"
#include "widget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForLocale(codec);
    QTextCodec::setCodecForCStrings(codec);
    QTextCodec::setCodecForTr(codec);

    MyInputPanelContext *ic = new MyInputPanelContext;
    a.setInputContext(ic);

    Widget w;
    w.show();

    return a.exec();
}
