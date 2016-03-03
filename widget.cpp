#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QtCore>
#include <QtSql>
#include <QtNetwork>
#include <QMessageBox>
#include "videodevice.h"

extern "C"
{
    #include <stdio.h>
    #include <stdlib.h>
}

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    QTextCodec::setCodecForTr(QTextCodec::codecForLocale());
    ui->setupUi(this);
    sendStart = false;
    tcpStart = false;
    serveron = false;

    //database operations
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("test.db");
    if(!db.open())
    {
        qDebug()<<"open data base failed";
    }
    QSqlQuery query;
    query.exec("DROP TABLE IF EXISTS data");
    query.exec("CREATE TABLE IF NOT EXISTS data (_id INTEGER PRIMARY KEY AUTOINCREMENT, time INTEGER(14), val INTEGER)");

    query.exec("DROP TABLE IF EXISTS port");
    bool test = query.exec("CREATE TABLE IF NOT EXISTS port(_id INTEGER PRIMARY KEY AUTOINCREMENT, name VARCHAR, path VARCHAR, baudrate VARCHAR, databits VARCHAR, command VARCHAR)");
    //qDebug() << test;
    test = query.exec("INSERT INTO port VALUES (NULL,'ttyUSB0','/dev/ttyUSB0','4800','8','010300020003a40b')");
    //qDebug() << test;

    refreshPortTable();

    query.exec("INSERT INTO data VALUES (NULL,20080101000000,5673)");
    query.exec("INSERT INTO data VALUES (NULL,20090101000000,1234)");
    query.exec("INSERT INTO data VALUES (NULL,20100101000000,3465)");

    //set reading delay
    readDelay = 20000;

    ui->tabWidget->setCurrentIndex(0);

    clientConnection = '\0';

    readTimer = new QTimer(this);//timer for read opertaion
    //readTimer->start(100);
    connect(readTimer,SIGNAL(timeout()),this,SLOT(readMyCom()));
    //connect timer to slot

    msgTimer = new QTimer(this);//timer for send operation
    connect(msgTimer, SIGNAL(timeout()), this, SLOT(writeHexData()));

    imgTimer = new QTimer(this);
    connect(imgTimer,SIGNAL(timeout()),this,SLOT(sendframe()));

    ipTimer = new QTimer(this);
    connect(ipTimer, SIGNAL(timeout()), this, SLOT(sendip()));

    //get server ip
    QHostAddress add;
    foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost))
        {
            qDebug() << address.toString();
            add = address;
            ui->labelsip->setText(QString("address: ").append(address.toString()));
        }
    }

    on_pushButtonser_clicked();

    hostip = ui->lineEditip->text();
    hostpt = ui->lineEditpt->text();

}

Widget::~Widget()
{
    db.close();
    delete ui;
}

//chuan kou du qu
void Widget::readMyCom() //read data from serial port
{   
    QByteArray temp = myCom->readAll();//read data
    if(temp.length() > 0)
    {
        double val = (((unsigned char)temp.at(3))*256 + (unsigned char)temp.at(4))/(double)1000;//convert data into double
        QDateTime tt = QDateTime::currentDateTime(); //get date info
        //display data in the browser
        QString str = tt.toString("yyyyMMddhhmmss");

        ui->labelport->setText(QString("At ").append(str).append(" value is: ").append(QString::number(val,'f', 3)));

        //insert data into database
        QSqlQuery query;
        QString exeq = QString("INSERT INTO data VALUES (NULL, '").append(str).append("', %1)").arg(val);
        //qDebug() << exeq;
        bool ret = query.exec(exeq);
        qDebug() << ret;
        if(ui->tabWidget->currentIndex() == 2){
            refreshSqlView();
        }


        //send data using tcpclient
        if(tcpStart == true){
            tcpSocket = new QTcpSocket;
            tcpSocket->connectToHost(QHostAddress(ui->lineEditip->text()), ui->lineEditpt->text().toInt());
            array = new QByteArray;
            array->clear();
            array->append("at ");
            array->append(str);
            array->append(" value is ");
            array->append(QString::number(val,'f', 3));
            connect(tcpSocket, SIGNAL(connected()), this, SLOT(tcp_send()));
        }

        //debuging output
        QString show;
        for(int i=0; i<temp.length(); i++){
            show.append(QString::number((unsigned char)temp.at(i), 16));
            show.append(" ");
        }
        qDebug() << "received: " << show;
        readTimer->stop();
    }
    else
    {
        //qDebug() << "received: nothing";
        writeHexData();//if no respond received, resend data
    }
}

//wei zhi fu wu qi. wei ding yi ge shi
void Widget::tcp_send(){
    //write data to socket and display status.
    if(array == NULL){
        return;
    }
    tcpSocket->write(array->data());
    QDateTime tt = QDateTime::currentDateTime(); //date info
    QString str = tt.toString("yyyy-mm-dd hh:mm:ss dddd");
    ui->timelable->setText(str);//display data in the lable
}

void Widget::sendip(){
    ipsoc = new QTcpSocket;
    ipsoc->connectToHost(QHostAddress(hostip), hostpt.toInt());
    servmsg = new QByteArray;
    servmsg->clear();
    servmsg->append(ui->labelsip->text());
    servmsg->append(":");
    servmsg->append(ui->lineEditport->text());
    qDebug() << "trying to send: " << servmsg->data();
    connect(ipsoc, SIGNAL(connected()), this, SLOT(writetoServer()));
}

void Widget::writetoServer(){
    ipsoc->write(servmsg->data());
    qDebug() << "send success.";
    ipsoc->disconnect();
}

//chuan kou xie ru
void Widget::writeHexData(){//write data to serial port

    QString str = "010300020003a40b";
    if(str.length() == 0 || (str.length())%2 == 1)
    {
        qDebug() << "data format error";
        return;
    }

    QByteArray data;

    String2Hex(str, data);
    myCom->write(data.data(), data.length());

    QString show;
    for(int i=0; i<data.length(); i++){
        show.append(QString::number((unsigned char)data.data()[i], 16));
        //show.append(QString::number((unsigned char)data.at(i), 16));
        show.append(" ");
    }
    qDebug() << "sending: " << show;

    readTimer->start(1000);//start reading from port

    //write out
}

//chuan kou an niu cao han shu
void Widget::on_pushButton_clicked()  //slot function
{
    if(sendStart == false)
    {
        QSqlQuery query;
        query.exec("SELECT * FROM port");
        while(query.next()){
            qDebug() << "port: " << query.value(2).toString();
        }

        myCom = new Posix_QextSerialPort("/dev/ttyUSB0",QextSerialBase::Polling);
        //connect to port using Polling method
        qDebug() << "connecting port";
        bool bol = myCom->open(QIODevice::ReadWrite);//open port
        if(bol == false){
            return;
        }

        //open port
        myCom->setBaudRate(BAUD4800);
        //set rate
        myCom->setDataBits(DATA_8);
        //8 bit
        myCom->setParity(PAR_NONE);
        //parity
        myCom->setStopBits(STOP_1);
        //stop bit
        myCom->setFlowControl(FLOW_OFF);
        //flow control
        myCom->setTimeout(10);
        //buffer delay

        writeHexData();

        sendStart = true;
        ui->pushButton->setText("stop");
        msgTimer->start(readDelay);//strat send message to port
    }
    else
    {
        readTimer->stop();//stop reading
        msgTimer->stop();//stop sending
        sendStart = false;
        ui->pushButton->setText("start");
    }
}

//kai qi / guan bi tcp fu wu
void Widget::on_pushButtontcp_clicked()//slot function
{
    if(tcpStart == false){
        tcpStart = true;
        ui->pushButtontcp->setText("disable");
        ui->lineEditip->setEnabled(false);
        ui->lineEditpt->setEnabled(false);
    }
    else{
        tcpStart = false;
        ui->pushButtontcp->setText("enable");
        ui->lineEditip->setEnabled(true);
        ui->lineEditpt->setEnabled(true);
    }
}

//shou dong fa song ip di zhi
void Widget::on_pushButtonsendip_clicked()
{
    if(ui->pushButtonsendip->text() == QString("send")){
        hostip = ui->lineEditip->text();
        hostpt = ui->lineEditpt->text();
        ipTimer->start(7200000);
        ui->pushButtonsendip->setText(QString("stop"));
    }
    else{
        ipTimer->stop();
        ui->pushButtonsendip->setText(QString("send"));
    }
}

//shua xin ben di shu ju
void Widget::on_tabWidget_selected(QString )//slot function
{
    if(ui->tabWidget->currentIndex() == 2){
        refreshSqlView();
    }
}

//shua xin shu ju
void Widget::refreshSqlView()//display form from database
{
    /*db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("test.db");
    if(!db.open())
    {
        qDebug()<<"open data base failed";
    }*/
    /*model = new QSqlQueryModel(ui->tableView);
    model->setQuery(QString("SELECT * FROM data"));
    model->setHeaderData(0, Qt::Horizontal, QString("id"));
    model->setHeaderData(1, Qt::Horizontal, QString("time"));
    model->setHeaderData(2, Qt::Horizontal, QString("value"));
    ui->tableView->setModel(model);*/

    tmodel = new QSqlTableModel(this);
    tmodel->setTable("data");
    tmodel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    tmodel->select();
    tmodel->setHeaderData(0, Qt::Horizontal, QString("id"));
    tmodel->setHeaderData(1, Qt::Horizontal, QString("time"));
    tmodel->setHeaderData(2, Qt::Horizontal, QString("value"));
    ui->tableView->setModel(tmodel);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->setColumnHidden(0, true);
}

//shua xin chuan kou lie biao
void Widget::refreshPortTable(){
    QSqlTableModel* pt = new QSqlTableModel(this);
    pt->setTable("port");
    pt->setEditStrategy(QSqlTableModel::OnManualSubmit);
    pt->select();
    pt->setHeaderData(0, Qt::Horizontal, QString("id"));
    pt->setHeaderData(1, Qt::Horizontal, QString("name"));
    pt->setHeaderData(2, Qt::Horizontal, QString("path"));
    pt->setHeaderData(3, Qt::Horizontal, QString("baudrate"));
    pt->setHeaderData(4, Qt::Horizontal, QString("bits"));
    pt->setHeaderData(5, Qt::Horizontal, QString("command"));
    ui->tableViewport->setModel(pt);
    ui->tableViewport->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableViewport->setColumnHidden(0, true);
    ui->tableViewport->setColumnHidden(2, true);
    ui->tableViewport->setColumnHidden(5, true);
}

//shan chu yi tiao shu ju
void Widget::on_tableView_clicked(QModelIndex index)//slot function
{
    ui->tableView->selectRow(index.row());
    tmodel->removeRow(index.row());
    int sure = QMessageBox::warning(this, QString("Delete this row"), "Are you sure to remove this row from the database?", QMessageBox::Yes, QMessageBox::No);
    if(sure == QMessageBox::No)
    {
        tmodel->revertAll();
    }
    else tmodel->submitAll();
}

//fu wu qi kai qi / guan bi
void Widget::on_pushButtonser_clicked()//slot function
{
    if(serveron == false){
        tcpServer = new QTcpServer(this);
        if(!tcpServer->listen(QHostAddress::Any, ui->lineEditport->text().toInt())){
            qDebug() << tcpServer->errorString();
            close();
        }
        serveron = true;
        ui->pushButtonser->setText("disable");
        ui->lineEditport->setEnabled(false);
        ui->labelserst->setText("Wait for connection");
        connect(tcpServer, SIGNAL(newConnection()), this, SLOT(newCon()));
    }
    else{
        if(clientConnection != '\0'){
            clientConnection->close();
        }
        tcpServer->close();
        serveron = false;
        ui->lineEditport->setEnabled(true);
        ui->labelserst->setText("Server is off");
        ui->pushButtonser->setText("enable");
    }
}

//xin ke hu duan lian jie
void Widget::newCon(){
    clientConnection = tcpServer->nextPendingConnection();
    ui->labelserst->setText(QString("Client ").append(clientConnection->peerAddress().toString()).append(" connected."));
    connect(clientConnection, SIGNAL(readyRead()), this, SLOT(processRequest()));
    connect(clientConnection, SIGNAL(disconnected()), this, SLOT(clientdown()));
}

//ke hu duan duan kai
void Widget::clientdown(){
    ui->labelserst->setText(QString("Wait for connection"));
    ui->labelreq->setText(QString(" "));
    if(ui->pushButtonsendimg->text() == QString("stop")){
        on_pushButtonsendimg_clicked();
    }
}

//chu li ke hu duan zhi ling
void Widget::processRequest(){
    /*
    return code:
    0 success
    1 unknown command
    2 request service already on
    5 enable/disable service failed
    */
    msg = clientConnection->readAll();
    ui->labelreq->setText(QString("request received: ").append(msg));

    if(msg == QString("PRT O")){
        if(sendStart == false){
            on_pushButton_clicked();
            if(sendStart == false){
                writetoClient("5");
            }
            else
            {
                writetoClient("0");
            }
        }
        else{
            writetoClient("2");
        }
    }
    else if(msg == "CAM O"){
        on_pushButtoncap_clicked();
    }
    else if(msg == "VID O"){
        if(ui->pushButtonsendimg->text() == QString("start")){
            on_pushButtonsendimg_clicked();
            writetoClient("0");
        }
        else{
            writetoClient("2");
        }

    }
    else if(msg == "VID C"){
        if(ui->pushButtonsendimg->text() == QString("stop")){
            on_pushButtonsendimg_clicked();
            writetoClient("0");
        }
        else{
            writetoClient("2");
        }

    }
    else if(msg == QString("PRT C")){
        if(sendStart == true){
            on_pushButton_clicked();
            if(sendStart == true){
                writetoClient("5");
            }
            else
            {
                writetoClient("0");
            }
        }
        else{
            writetoClient("2");
        }
    }
    else if(msg == QString("TCP O")){
        if(tcpStart == false){
            on_pushButtontcp_clicked();
            if(tcpStart == false){
                writetoClient("5");
            }
            else {
                writetoClient("0");
            }
        }
        else{
            writetoClient("2");
        }
    }
    else if(msg == QString("TCP C")){
        if(tcpStart == true){
            on_pushButtontcp_clicked();
            if(tcpStart == true){
                writetoClient("5");
            }
            else {
                writetoClient("0");
            }
        }
        else{
            writetoClient("2");
        }
    }
    else if(msg.startsWith("DAT ")){
        QStringList sp = msg.split(" ");
        if(sp.length() == 3){
            if(QString(sp.at(1)).length() != 14 || QString(sp.at(2)).length() != 14){
                writetoClient("1");
                return;
            }
            int value = 0;
            QSqlQuery query;
            query.exec("SELECT * FROM data WHERE time BETWEEN " + QString(sp.at(1)) + " AND " + QString(sp.at(2)));
            while(query.next()){
                value += query.value(2).toInt();
            }
            qDebug() << value;
            writetoClient("RT " + QString::number(value, 10));
            return;
        }
        else{
            writetoClient("1");
        }
    }
    else{
        writetoClient("1");
    }
}

//xiang ke hu duan fa song zi fu chuan
void Widget::writetoClient(QString str){
    msg = str;
    sendstrbytcp();
}

//xiang ke hu duan fa song tu pian
void Widget::writetoClient(QImage img){
    imag = img;
    if(clientConnection->isOpen()){
        sendimgbytcp();
    }
    else{
        qDebug() << "no connection.";
    }
}

//zhuan huan han shu
void Widget::String2Hex(QString str, QByteArray &senddata)//convert string to hex string
{
    int hexdata,lowhexdata;
    int hexdatalen = 0;
    int len = str.length();
    senddata.resize(len/2);
    char lstr,hstr;
    for(int i=0; i<len; )
    {
        //char lstr,
        hstr=str[i].toAscii();
        if(hstr == ' ')
        {
            i++;
            continue;
        }
        i++;
        if(i >= len)
            break;
        lstr = str[i].toAscii();
        hexdata = ConvertHexChar(hstr);
        lowhexdata = ConvertHexChar(lstr);
        if((hexdata == 16) || (lowhexdata == 16))
            break;
        else
            hexdata = hexdata*16+lowhexdata;
        i++;
        senddata[hexdatalen] = (char)hexdata;
        hexdatalen++;
    }
    senddata.resize(hexdatalen);
}

//zhuan huan han shu
char Widget::ConvertHexChar(char ch)//convert hex number to char
{
    if((ch >= '0') && (ch <= '9'))
        return ch-0x30;
    else if((ch >= 'A') && (ch <= 'F'))
        return ch-'A'+10;
    else if((ch >= 'a') && (ch <= 'f'))
        return ch-'a'+10;
    else return (-1);
}

//zhao xiang gong neng
void Widget::on_pushButtoncap_clicked()
{
    int ret = opencam();

    if(ret == -1){
        qDebug() << "can not open camera.";
        return;
    }

    int i=5;
    while(i>0){
        getframe();
        i--;
    }

    writetoClient(*frame);
    frame->save("image.jpg");

    closecam();
}

//tong guo she xiang tou de dao 1 zhen
void Widget::getframe(){
    rs = vd->get_frame((void **)&p,&len);
    convert_yuv_to_rgb_buffer(p,pp,640,480);
    frame->loadFromData((uchar *)pp,640 * 480 * 3 * sizeof(char));
    //convert_yuv_to_rgb_buffer(p,pp,320,240);
    //frame->loadFromData((uchar *)pp,320 * 240 * 3 * sizeof(char));
    QPixmap tempmp = QPixmap::fromImage(*frame,Qt::AutoColor).scaled(ui->labelimg->width(), ui->labelimg->height(), Qt::KeepAspectRatio);
    ui->labelimg->setPixmap(tempmp);

    qDebug() << "picture taken";

    rs = vd->unget_frame();
}

//kai qi she xiang tou
int Widget::opencam(){
    pp = (unsigned char *)malloc(640 * 480 * 3 * sizeof(char));
    //pp = (unsigned char *)malloc(320 * 240 * 3 * sizeof(char));
    frame = new QImage(pp,640,480,QImage::Format_RGB888);
    //frame = new QImage(pp,320,240,QImage::Format_RGB888);

    vd = new VideoDevice(tr("/dev/video0"));

    rs = vd->open_device();

    if(-1==rs)
    {
        //QMessageBox::warning(this,tr("error"),tr("open /dev/dsp error"),QMessageBox::Yes);
        //QMessageBox::warning(this,tr("error"),tr("打开设备失败"),QMessageBox::Yes);
        vd->close_device();
        return rs;
    }

    rs = vd->init_device();
    if(-1==rs)
    {
        //QMessageBox::warning(this,tr("error"),tr("init failed"),QMessageBox::Yes);
        vd->close_device();
        return rs;
    }

    rs = vd->start_capturing();
    if(-1==rs)
    {
        //QMessageBox::warning(this,tr("error"),tr("start capture failed"),QMessageBox::Yes);
        vd->close_device();
        return rs;
    }

    if(-1==rs)
    {
        //QMessageBox::warning(this,tr("error"),tr("get frame failed"),QMessageBox::Yes);
        vd->stop_capturing();
        return rs;
    }
    return 1;
}

//guan bi she xiang tou
void Widget::closecam(){
    rs = vd->stop_capturing();
    rs = vd->uninit_device();
    rs = vd->close_device();
}

int Widget::convert_yuv_to_rgb_buffer(unsigned char *yuv, unsigned char *rgb, unsigned int width, unsigned int height)
{
 unsigned int in, out = 0;
 unsigned int pixel_16;
 unsigned char pixel_24[3];
 unsigned int pixel32;
 int y0, u, y1, v;
 for(in = 0; in < width * height * 2; in += 4) {
  pixel_16 =
   yuv[in + 3] << 24 |
   yuv[in + 2] << 16 |
   yuv[in + 1] <<  8 |
   yuv[in + 0];
  y0 = (pixel_16 & 0x000000ff);
  u  = (pixel_16 & 0x0000ff00) >>  8;
  y1 = (pixel_16 & 0x00ff0000) >> 16;
  v  = (pixel_16 & 0xff000000) >> 24;
  pixel32 = convert_yuv_to_rgb_pixel(y0, u, v);
  pixel_24[0] = (pixel32 & 0x000000ff);
  pixel_24[1] = (pixel32 & 0x0000ff00) >> 8;
  pixel_24[2] = (pixel32 & 0x00ff0000) >> 16;
  rgb[out++] = pixel_24[0];
  rgb[out++] = pixel_24[1];
  rgb[out++] = pixel_24[2];
  pixel32 = convert_yuv_to_rgb_pixel(y1, u, v);
  pixel_24[0] = (pixel32 & 0x000000ff);
  pixel_24[1] = (pixel32 & 0x0000ff00) >> 8;
  pixel_24[2] = (pixel32 & 0x00ff0000) >> 16;
  rgb[out++] = pixel_24[0];
  rgb[out++] = pixel_24[1];
  rgb[out++] = pixel_24[2];
 }
 return 0;
}

int Widget::convert_yuv_to_rgb_pixel(int y, int u, int v)
{
 unsigned int pixel32 = 0;
 unsigned char *pixel = (unsigned char *)&pixel32;
 int r, g, b;
 r = y + (1.370705 * (v-128));
 g = y - (0.698001 * (v-128)) - (0.337633 * (u-128));
 b = y + (1.732446 * (u-128));
 if(r > 255) r = 255;
 if(g > 255) g = 255;
 if(b > 255) b = 255;
 if(r < 0) r = 0;
 if(g < 0) g = 0;
 if(b < 0) b = 0;
 pixel[0] = r * 220 / 256;
 pixel[1] = g * 220 / 256;
 pixel[2] = b * 220 / 256;
 return pixel32;
}

//kai qi / guan bi shi ping
void Widget::on_pushButtonsendimg_clicked()
{
    if(clientConnection == NULL){
        qDebug() << "not init";
        return;
    }
    if(clientConnection->isReadable()){
        qDebug() << "is open";
    }
    else{
        qDebug() << "not open";
        return;
    }
    if(ui->pushButtonsendimg->text() == QString("start")){
        int ret =opencam();
        if(ret == -1){
            qDebug() << "can not open camera.";
            return;
        }
         ui->pushButtonsendimg->setText("stop");
        imgTimer->start(500);
    }
    else{
        imgTimer->stop();
        ui->pushButtonsendimg->setText("start");
        qDebug() << "timer stopped.";
        closecam();
        qDebug() << "cam closed.";
    }
}

//fa song tu pian
void Widget::sendframe(){
    getframe();
    writetoClient(*frame);
}

//fa song zi fu chuan shu ju jie gou
void Widget::sendstrbytcp(){
    QByteArray ba;
    QDataStream out(&ba, QIODevice::WriteOnly);
    out << (quint64) 0;
    out << QString("STR");
    out << msg;
    out.device()->seek(0);
    out << (quint64)(ba.size()-sizeof(quint64));
    clientConnection->write(ba);
    qDebug() << "str sent: " << ba.toHex();
}

//fa song tu pian shu ju jie gou
void Widget::sendimgbytcp(){
    QByteArray ba;
    QDataStream out(&ba, QIODevice::WriteOnly);
    out << (quint64) 0;
    out << QString("IMG");
    out << imag;
    out.device()->seek(0);
    quint64 trytry = ba.size()-sizeof(quint64);
    out << (quint64)(ba.size()-sizeof(quint64));
    clientConnection->write(ba);
    qDebug() << "str sent: " << ba.size()-sizeof(quint16) << " " << trytry;
}
