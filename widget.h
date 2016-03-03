#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "posix_qextserialport.h"
#include "videodevice.h"
#include <QTimer>
#include <QtNetwork>
#include <QtSql>

namespace Ui {
    class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

private:
    Ui::Widget *ui;
    Posix_QextSerialPort *myCom;
    QTimer *readTimer;
    QTimer *msgTimer;
    bool sendStart;
    int readDelay;
    //for tcpClient
    bool tcpStart;
    QTcpSocket *tcpSocket;
    quint16 blockSize;
    //for database
    QSqlDatabase db;
    QSqlTableModel *tmodel;
    QByteArray *array;
    //for tcpServer
    QTcpServer *tcpServer;
    QTcpSocket *clientConnection;
    QTcpSocket *ipsoc;
    QString msg;
    QImage imag;
    bool serveron;
    //camera
    QImage *frame;
    QTimer *timer;
    QTimer *imgTimer;
    QTimer *ipTimer;
    int rs;
    uchar *pp;
    uchar * p;
    unsigned int len;
    int convert_yuv_to_rgb_pixel(int y, int u, int v);
    int convert_yuv_to_rgb_buffer(unsigned char *yuv, unsigned char *rgb, unsigned int width, unsigned int height);
    VideoDevice *vd;
    QString hostip;
    QString hostpt;
    QByteArray* servmsg;


private slots:
    void on_pushButtonsendimg_clicked();
    void on_pushButtoncap_clicked();
    void on_pushButtonsendip_clicked();
    void on_pushButtonser_clicked();
    void on_tableView_clicked(QModelIndex index);
    void on_tabWidget_selected(QString );
    void on_pushButtontcp_clicked();
    void on_pushButton_clicked();
    void readMyCom();
    void String2Hex(QString, QByteArray&);
    char ConvertHexChar(char);
    void writeHexData();
    void tcp_send();
    void refreshSqlView();
    void newCon();
    void clientdown();
    void processRequest();
    void refreshPortTable();
    void sendimgbytcp();
    void sendstrbytcp();
    void writetoClient(QString);
    void writetoClient(QImage);
    void closecam();
    int opencam();
    void getframe();
    void sendframe();
    void sendip();
    void writetoServer();
};

#endif // WIDGET_H
