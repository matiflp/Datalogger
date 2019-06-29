/*
 *  datalogger.cpp
 *
 *  Created on: 2019
 *  Authors:    Matias Lopez
 *              Jesus Lopez
 *
*/

#include "datalogger.h"
#include "ui_datalogger.h"

Sensor::Sensor()
{
    date.year = 0;
    date.month = 0;
    date.day = 0;
    date.hour = 0;
    date.min = 0;
    date.sec = 0;
}

Sensor::~Sensor()
{
}

// almacena la fecha enviada por arduino
void Sensor::setDate(const QByteArray& data, const int i)
{
    quint16 conversion = 0;
    date.hour = quint8 (data.at(i+2));
    date.min = quint8 (data.at(i+3));
    date.sec = quint8 (data.at(i+4));
    date.day = quint8 (data.at(i+5));
    date.month = quint8 (data.at(i+6));
    conversion = quint8 (data.at(i+7));
    conversion = quint16 (conversion << 8);
    conversion |= quint8 (data.at(i+8));
    date.year = conversion;
}

Temperature::Temperature()
{
    value = 0.0;
}

Temperature::~Temperature()
{
}

// almacena la temperatura enviada por arduino
void Temperature::setValue(const QByteArray& data, const int i)
{
    quint16 conversion;
    conversion = quint8 (data.at(i+9));
    conversion = quint16 (conversion << 8);
    conversion |= quint8 (data.at(i+10));
    value = (conversion * 500) / 1024;
}

Distance::Distance()
{
    value = 0.0;
}

Distance::~Distance()
{
}

// almacena la distancia enviada por arduino
void Distance::setValue(const QByteArray& data, int i)
{
    quint32 duration = 0;
    duration = quint8 (data.at(i+11));
    duration = quint32 (duration << 8);
    duration |= quint8 (data.at(i+12));
    duration = quint32 (duration << 8);
    duration |= quint8 (data.at(i+13));
    duration = quint32 (duration << 8);
    duration |= quint8 (data.at(i+14));

    value = float (duration * 0.017);
}

Communication::Communication()
{
    headerL = qint8 (0xFE);
    headerH = qint8 (0xFF);
    stopt = qint8 (0xFE);
    arduino_uno_vendor_id = 6790;
    arduino_uno_product_id = 29987;
    arduino_is_available = false;
    arduino_port_name = "";
    arduino = new QSerialPort;
}

Communication::~Communication()
{
    closeSerialPort();
    delete arduino;
}

// conecta con el arduino, abre y configura el puerto serie
bool Communication::startSerialPort()
{
    bool connection = 1;

    foreach(const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts())
    {
        if(serialPortInfo.hasVendorIdentifier() && serialPortInfo.hasProductIdentifier())
        {
            if(serialPortInfo.vendorIdentifier() == arduino_uno_vendor_id)
            {
                if(serialPortInfo.productIdentifier() == arduino_uno_product_id)
                {
                    arduino_port_name = serialPortInfo.portName();
                    arduino_is_available = true;
                }
            }
        }
    }

    if(arduino_is_available)
    {
        // open and configure the serialport
        arduino->setPortName(arduino_port_name);
        if(!arduino->open(QIODevice::ReadWrite))connection = 0;
        arduino->setBaudRate(QSerialPort::Baud9600);
        arduino->setDataBits(QSerialPort::Data8);
        arduino->setParity(QSerialPort::NoParity);
        arduino->setStopBits(QSerialPort::OneStop);
        arduino->setFlowControl(QSerialPort::NoFlowControl);
    }else
    {
        connection = 0;
    }
    return connection;
}

// realiza la lectura
void Communication::readData()
{
    data = arduino->readAll();
}

// envia un "1" al arduino para decirle que comience a enviar datos
bool Communication::writeToStart()
{
    bool connection;
    if(arduino_is_available && arduino->isWritable()){
        arduino->write("1");
        connection = true;
    }else{
        connection = false;
    }
    return connection;
}

// envia un "0" al arduino para decirle que deje de enviar datos
bool Communication::writeToStop()
{
    bool connection;
    if(arduino_is_available && arduino->isWritable()){
        arduino->write("0");
        connection = true;
    }else{
        connection = false;
    }
    return connection;
}

Datalogger::Datalogger(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Datalogger)
{
    ui->setupUi(this);

    QString NameDb;
    NameDb.append("DataLoggerDataBase.sqlite");
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(NameDb);
    if(!db.open())
    {
        QMessageBox::critical(this,tr("error::"),db.lastError().text());
    }
    createTable();

    setLedRed();

    timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(onTimeOut()));

    connect(communication.getArduino(),SIGNAL(readyRead()), this,SLOT(decoFrame()));

    ui -> progressBar -> hide();
    ui -> progressBar -> setValue(0);

    ui -> comboBox -> addItem("Forma");
    ui -> comboBox -> addItem("Ascendente");
    ui -> comboBox -> addItem("Descendente");
}

Datalogger::~Datalogger()
{
    db.close();
    delete timer;
    delete ui;
}

// se crea dos tablas en la base de datos para almacenar los valores de temperatura y distancia
void Datalogger::createTable()
{
    QString temperature;
    temperature.append("CREATE TABLE IF NOT EXISTS dataLoggerTemp("
                    "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "Fecha VARCHAR(50),"
                    "Temperatura FLOAT NOT NULL"
                    ");");

    QSqlQuery dataLoggerTemp;
    dataLoggerTemp.prepare(temperature);
    if(!dataLoggerTemp.exec())
    {
        QMessageBox::critical(this,tr("error::"),dataLoggerTemp.lastError().text());
    }

    QString distance;
    distance.append("CREATE TABLE IF NOT EXISTS dataLoggerDist("
                    "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "Fecha VARCHAR(50),"
                    "Distancia INTEGER NOT NULL"
                    ");");

    QSqlQuery dataLoggerDist;
    dataLoggerDist.prepare(distance);

    if(!dataLoggerDist.exec())
    {
        QMessageBox::critical(this,tr("error::"),dataLoggerDist.lastError().text());
    }

    ui -> tableWidget -> setColumnWidth(0,25);
    ui -> tableWidget -> setColumnWidth(1,135);
}

// se insertan los valores recibidos de temperatura y distancia en las tablas
void Datalogger::insertData()
{
    qint8 i = 0;
    QSqlQuery insertTemp;
    Date dateComplete = temp.getDate();
    QString dateComp1;
    dateComp1 = QString::number(dateComplete.day) + "/" + QString::number(dateComplete.month) + "/" +QString::number(dateComplete.year) + (dateComplete.hour>9?" ":" 0") +
                 QString::number(dateComplete.hour) + (dateComplete.min>9?":":":0") +QString::number(dateComplete.min) + (dateComplete.sec>9?":":":0") +QString::number(dateComplete.sec);
    insertTemp.prepare("INSERT INTO dataLoggerTemp (Fecha, Temperatura) VALUES (:dateComp1,:temp);");
    insertTemp.bindValue(":dateComp1", dateComp1);
    insertTemp.bindValue(":temp", temp.getValue());

    if(insertTemp.exec())
    {
        i++;
    }else
    {
        QMessageBox::critical(this,tr("error::"),insertTemp.lastError().text());
    }

    // insertar datos en la tabla de distancia.
    QSqlQuery insertDist;
    insertDist.prepare("INSERT INTO dataLoggerDist (Fecha, Distancia) VALUES (:dateComp1,:dist);");
    insertDist.bindValue(":dateComp1", dateComp1);
    insertDist.bindValue(":dist", qint16(dist.getValue()));

    if(insertDist.exec())
    {
        i++;
    }else
    {
        QMessageBox::critical(this,tr("error::"),insertDist.lastError().text());
    }

    if(i == 2)ui -> label_7 -> setText("Se han guardado los datos correctamente.");
}

// muestra los datos almacenados en la base de datos en una tablewidget
void Datalogger::showData(const QString name)
{
    QString consult, valDate;
    consult.append("SELECT * FROM ");
    consult.append(QString("%1").arg(name));
    QSqlQuery query;
    query.prepare(consult);

    if(query.exec())
    {
        ui -> label_7 -> setText("Datos Mostrados.");
    }else
    {
        QMessageBox::critical(this,tr("error::"),query.lastError().text());
    }

    if(name[10]=='D'){
        valDate = "Distancia";
    }else {
        valDate = "Temperatura";
    }

    // se cambia el nombre en funcion de que tabla que se esta accediendo
    QStringList l;
    l << "ID" << "Fecha" << valDate;
    ui -> tableWidget -> setHorizontalHeaderLabels(l);
    ui -> tableWidget -> setColumnWidth(0,25);
    ui -> tableWidget -> setColumnWidth(1,135);

    int row = 0;
    ui -> tableWidget -> setRowCount(0);

    while (query.next())
    {
        ui -> tableWidget -> insertRow(row);
        ui -> tableWidget -> setItem(row,0,new QTableWidgetItem(query.value(0).toByteArray().constData()));
        ui -> tableWidget -> setItem(row,1,new QTableWidgetItem(query.value(1).toByteArray().constData()));
        ui -> tableWidget -> setItem(row,2,new QTableWidgetItem(query.value(2).toByteArray().constData()));
        row++;
    }

}

// se ordena los datos en forma ascendente o descendente
void Datalogger::sortData(const QString name, const QString type)
{
    QString order, valDate;

    if(name[10]=='D'){
        valDate = "Distancia";
    }else {
        valDate = "Temperatura";
    }

    order.append("SELECT * FROM ");
    order.append(QString("%1").arg(name));
    order.append(" ORDER BY ");
    order.append(QString("%1").arg(valDate));
    order.append(" ");
    order.append(QString("%1").arg(type));
    QSqlQuery sort;
    sort.prepare(order);

    if(sort.exec())
    {
         ui -> label_7 -> setText("Los datos se han ordenado correctamente.");
    }else
    {
        QMessageBox::critical(this,tr("error::"),sort.lastError().text());
    }

    QStringList l;
    l << "ID" << "Fecha" << valDate ;
    ui -> tableWidget -> setHorizontalHeaderLabels(l);
    ui -> tableWidget -> setColumnWidth(0,25);
    ui -> tableWidget -> setColumnWidth(1,135);

    int row = 0;
    ui -> tableWidget -> setRowCount(0);
    while (sort.next())
    {
        ui -> tableWidget -> insertRow(row);
        ui -> tableWidget -> setItem(row,0,new QTableWidgetItem(sort.value(0).toByteArray().constData()));
        ui -> tableWidget -> setItem(row,1,new QTableWidgetItem(sort.value(1).toByteArray().constData()));
        ui -> tableWidget -> setItem(row,2,new QTableWidgetItem(sort.value(2).toByteArray().constData()));
        row++;
    }
}

// busca un dato almacenado en la base de datos
void Datalogger::browserData(const QString name, const QString browse)
{
    QString search, valDate;
    if(name[10]=='D'){
        if (ui -> radioButton_3 -> isChecked()){
            valDate = "Distancia";
        }else if (ui -> radioButton_4 -> isChecked()) {
            valDate = "Fecha";
        }
    }else{
        if (ui -> radioButton_3 -> isChecked()){
            valDate = "Temperatura";
        }else if (ui -> radioButton_4 -> isChecked()) {
            valDate = "Fecha";
        }
    }
    search.append("SELECT * FROM ");
    search.append(QString("%1").arg(name));
    search.append(" WHERE ");
    search.append(QString("%1").arg(valDate));
    search.append("='");
    search.append(QString("%1").arg(browse));
    search.append("'");
    QSqlQuery searchFor;
    searchFor.prepare(search);

    if(!searchFor.exec())
    {
        QMessageBox::critical(this,tr("error::"),searchFor.lastError().text());
    }

    QStringList l;
    l << "ID" << "Fecha" << valDate ;
    ui -> tableWidget -> setHorizontalHeaderLabels(l);
    ui -> tableWidget -> setColumnWidth(0,25);
    ui -> tableWidget -> setColumnWidth(1,135);

    int row = 0;
    ui -> tableWidget -> setRowCount(0);
    while (searchFor.next())
    {
        ui -> tableWidget -> insertRow(row);
        ui -> tableWidget -> setItem(row,0,new QTableWidgetItem(searchFor.value(0).toByteArray().constData()));
        ui -> tableWidget -> setItem(row,1,new QTableWidgetItem(searchFor.value(1).toByteArray().constData()));
        ui -> tableWidget -> setItem(row,2,new QTableWidgetItem(searchFor.value(2).toByteArray().constData()));
        row ++;
    }
    // si fila es igual cero quiere decir que no se encontro el valor.
    if(row == 0){
        QMessageBox::critical(this, "ERROR", "No se ha encontrado el dato.");
    }
}

// borra el dato seleccionado de la base de datos
void Datalogger::deleteData(const QString name, const QString id)
{
    // primero obtenemos el numero de filas de la base de datos
    QString del;
    del.append("SELECT * FROM ");
    del.append(QString("%1").arg(name));
    QSqlQuery deleted;
    deleted.prepare(del);
    deleted.exec();
    int row = 0;
    while(deleted.next())row++;

    //  ahora borramos si no supera la cantidad de datos almacenada en la base de datos
    if( id.toInt() <=  row){
        QString del1;
        del1.append("DELETE FROM ");
        del1.append(QString("%1").arg(name));
        del1.append(" WHERE ID= ");
        del1.append(QString("%1").arg(id));
        QSqlQuery deleted1;
        deleted1.prepare(del1);

        if(deleted1.exec())
        {
            QMessageBox::critical(this,"Aviso","Dato eliminado.");
            ui -> label_7 -> setText("Dato Eliminado.");
        }else {
            QMessageBox::critical(this,tr("error::"),deleted1.lastError().text());
        }

        // por ultimo actualizamos los datos almacenados
        QString del2;
        QSqlQuery deleted2;
        deleted2.prepare("UPDATE '"+name+"' SET ID=ID-1 WHERE ID>=:ID");
        deleted2.bindValue(":ID", id);
        if(!deleted2.exec())
        {
            QMessageBox::critical(this,tr("Error"),deleted2.lastError().text());
        }
    }else {
        QMessageBox::critical(this,"Aviso","No es posible borrar. Supera la cantidad de datos almacenados.");
    }
}

// setea pushButton_3 en color verde
void Datalogger::setLedGreen()
{
    ui -> pushButton_3 -> setStyleSheet("* { background-color: rgb(87,200,057) }");
    ui -> pushButton_3 -> setText("Conectado");
}

// setea pushButton_3 en color rojo
void Datalogger::setLedRed()
{
    ui -> pushButton_3 -> setStyleSheet("* { background-color: rgb(255,006,005) }");
    ui -> pushButton_3 -> setText("Desconectado");
}

// slot para decodifcar la trama una vez emitida una señal de listo para leer
void Datalogger::decoFrame()
{

    QByteArray data;
    int i;

    communication.readData();
    data = communication.getData();
    ui ->progressBar -> setValue(75);

    if(data.size()>0)
    {

        i = data.indexOf(communication.getHeaderH(),0);

        if((data.at(i) == communication.getHeaderH()) && (data.at(i+1) == communication.getHeaderL()))
        {
            communication.writeToStop();
            temp.setDate(data,i);
            temp.setValue (data,i);
            dist.setDate(data,i);
            dist.setValue (data,i);

            insertData();
            ui ->progressBar -> hide();

            if(data.at(i+15) == communication.getStop())
            {
                ui -> progressBar -> setValue(100);
                ui -> label_7 -> setText("Se leyeron correctamente los datos.");
            }else {
                ui -> label_7 -> setText("Error en la lectura.");
            }

        }
    }

}

// slot para pedir el envio de datos al arduino una vez emitida la señal de tiempo expirado del timer
void Datalogger::onTimeOut()
{
    communication.writeToStart();
    ui -> progressBar -> show();
    ui -> progressBar -> setValue(25);
}

// boton comenzar
void Datalogger::on_pushButton_clicked()
{
    int ms = 0.0;
    if(communication.writeToStart()){
        ui -> label_7 -> setText("Comenzando lectura");
    }else {
        ui -> label_7 -> setText("No se pudo comenzar.");
    }
    ms = int(ui-> doubleSpinBox -> value()*60000);
    if(ms > 0.0)timer->start(ms);
    ui -> progressBar -> show();
    ui -> progressBar -> setValue(25);

}

// boton detener
void Datalogger::on_pushButton_2_clicked()
{   
    if(communication.writeToStop()){
        ui -> label_7 -> setText("Recepción de datos finalizada.");
    }else {
        ui -> label_7 -> setText("Error finalizando la recepción.");
    }
    timer->stop();
    ui -> progressBar -> setValue(0);
}

// boton conectar
void Datalogger::on_pushButton_4_clicked()
{
    if(!communication.startSerialPort()){
        QMessageBox::critical(this, "ERROR", "No se ha podido lograr la conexión con el Arduino.");
        setLedRed();
    }else {
        ui -> label_7 -> setText("Conectado al Arduino.");
        setLedGreen();
    }
}

// boton mostrar datos
void Datalogger::on_pushButton_5_clicked()
{
    if(ui -> radioButton -> isChecked()){
        QString name = "dataLoggerTemp";
        showData(name);
    }else if (ui -> radioButton_2 -> isChecked()) {
        QString name = "dataLoggerDist";
        showData(name);
    }
}

// boton desconectar
void Datalogger::on_pushButton_6_clicked()
{
    communication.closeSerialPort();
    setLedRed();
    ui -> label_7 -> setText("Se ha finalizado la comunicación con Arduino.");
}

// boton ordenar
void Datalogger::on_pushButton_9_clicked()
{
    QString name;
    QString type;

    if(ui -> radioButton -> isChecked()){
        if(ui -> radioButton_3 -> isChecked()){
            if(ui -> comboBox -> currentText() == "Ascendente"){
                name = "dataLoggerTemp";
                type = "ASC";
                sortData(name,type);
            }else{
                name = "dataLoggerTemp";
                type = "DESC";
                sortData(name,type);
            }
         }else if(ui -> radioButton_4 -> isChecked()){
            if (ui -> comboBox -> currentText() == "Ascendente") {
                name = "dataLoggerTemp";
                type = "ASC";
                sortData(name,type);
            }else{
                name = "dataLoggerTemp";
                type = "DESC";
                sortData(name,type);
            }
        }
    }else if(ui -> radioButton_2 -> isChecked()){
            if(ui -> radioButton_3 -> isChecked()){
                if(ui -> comboBox -> currentText() == "Ascendente"){
                    name = "dataLoggerDist";
                    type = "ASC";
                    sortData(name,type);
                }else {
                    name = "dataLoggerDist";
                    type = "DESC";
                    sortData(name,type);
                }
         }else if(ui -> radioButton_4 -> isChecked()){
                if(ui -> comboBox -> currentText() == "Ascendente"){
                    name = "dataLoggerDist";
                    type = "ASC";
                    sortData(name,type);
                }else {
                    name = "dataLoggerDist";
                    type = "DESC";
                    sortData(name,type);
                }
         }
    }
}

// boton buscar
void Datalogger::on_pushButton_10_clicked()
{
    QString name;

    if(ui -> radioButton -> isChecked()){
        if(ui -> radioButton_3 -> isChecked()){
            name = "dataLoggerTemp";
            browserData(name, ui -> lineEdit -> text());
        }else if (ui -> radioButton_4 -> isChecked()) {
            name = "dataLoggerTemp";
            browserData(name, ui -> lineEdit -> text());
        }

    }else if (ui -> radioButton_2 -> isChecked()) {
        if(ui -> radioButton_3 -> isChecked()){
            name = "dataLoggerDist";
            browserData(name, ui -> lineEdit -> text());
        }else if (ui -> radioButton_4 -> isChecked()) {
            name = "dataLoggerDist";
            browserData(name, ui -> lineEdit -> text());
        }
    }
}

// boton borrar
void Datalogger::on_pushButton_11_clicked()
{
    QString name;
    if(ui -> radioButton -> isChecked()){
        name = "dataLoggerTemp";
        deleteData(name, ui -> lineEdit_2 -> text());
    }else if (ui -> radioButton_2 -> isChecked()) {
        name = "dataLoggerDist";
        deleteData(name, ui -> lineEdit_2 -> text());
    }
}
