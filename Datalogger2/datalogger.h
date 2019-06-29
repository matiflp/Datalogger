/*
 *  datalogger.h
 *
 *  Created on: 2019
 *  Authors:    Matias Lopez
 *              Jesus Lopez
 *
*/

#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QMessageBox>
#include <QString>
#include <QDebug>
#include <QTimer>

namespace Ui {
class Datalogger;
}

class QSerialPort;
class QTimer;

struct Date
{
    quint16         year;
    quint8          month;
    quint8          day;
    quint8          hour;
    quint8          min;
    quint8          sec;
};

class Sensor
{
public:
                    Sensor          ();
    float           getValue        (void) const{return value;}
    Date            getDate         (void) const{return date;}
    void            setDate         (const QByteArray&, const int);
    virtual void    setValue        (const QByteArray&, const int) = 0;
    virtual         ~Sensor         ();
protected:
    float           value;
    Date            date;
};

class Temperature: public Sensor
{   
public:
                    Temperature     ();
    virtual void    setValue        (const QByteArray&, const int);
    virtual         ~Temperature    ();
};

class Distance: public Sensor
{ 
public:
                    Distance        ();
    virtual void    setValue        (const QByteArray&, const int);
    virtual         ~Distance       ();
};

class Communication: public QObject
{
public:
                    Communication   ();
    bool            startSerialPort ();
    void            closeSerialPort (){(arduino)->close();}
    bool            writeToStart    ();
    bool            writeToStop     ();
    void            readData        ();
    QByteArray      getData         () const {return data;}
    qint8           getHeaderH      () const {return headerH;}
    qint8           getHeaderL      () const {return headerL;}
    qint8           getStop         () const {return stopt;}
    QSerialPort*    getArduino      () const {return arduino;}
                    ~Communication  ();
private:
    QSerialPort     *arduino;
    quint16         arduino_uno_vendor_id;
    quint16         arduino_uno_product_id;
    QString         arduino_port_name;
    bool            arduino_is_available;
    qint8           headerL;
    qint8           headerH;
    qint8           stopt;
    QByteArray      data;
};

class Datalogger : public QMainWindow
{

    Q_OBJECT

public:
    explicit        Datalogger      (QWidget *parent = nullptr);
    void            createTable     ();
    void            insertData      ();
    void            showData        (const QString);
    void            sortData        (const QString, const QString);
    void            browserData     (const QString, const QString);
    void            deleteData      (const QString, QString);
    void            setLedRed       ();
    void            setLedGreen     ();
                    ~Datalogger     ();
public slots:
    void            decoFrame       ();
    void            onTimeOut       ();

private slots:
    void            on_pushButton_clicked   ();
    void            on_pushButton_2_clicked ();
    void            on_pushButton_4_clicked ();
    void            on_pushButton_5_clicked ();
    void            on_pushButton_6_clicked ();
    void            on_pushButton_9_clicked ();
    void            on_pushButton_10_clicked();
    void            on_pushButton_11_clicked();

private:
    Ui::Datalogger  *ui;
    QSqlDatabase    db;
    Temperature     temp;
    Distance        dist;
    Communication   communication;
    QTimer          *timer;
};


#endif // DATALOGGER_H
