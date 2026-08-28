#include "FileReader.h"
#include <QFile> //to open/close a file
#include <QTextStream> //to read a text line by line
#include <QDebug>


FileReader::FileReader() {
}

QList<User> FileReader::readUsers(){
    QList<User> users;
    QFile file("securegate/users.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
        qDebug() << "FILE COULD NOT BE OPENED";
        return users;
    }

    QTextStream in(&file);
    while (!in.atEnd()){
        QString idLine = in.readLine(); //first line is for id
        QString passwordLine = in.readLine(); //second line is for password

        QString id = idLine.mid(3); //discarding "id="
        QString password = passwordLine.mid(9); //discarding "password="

        users.append(User(id, password));
    }

    file.close();
    return users;
}