/*
 *  main.cpp
 *
 *  Created on: 2019
 *  Authors:    Matias Lopez
 *              Jesus Lopez
 *
*/

#include "datalogger.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Datalogger w;
    w.show();

    return a.exec();
}
