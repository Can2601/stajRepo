#include "FileReader.h"
#include <QFile> // to open/close a file
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

FileReader::FileReader() {}

QByteArray FileReader::readEncryptedFile(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "License file could NOT be opened:" << filePath;
        return QByteArray();
    }

    QByteArray data = file.readAll();
    file.close();
    return data;
}

QList<User> FileReader::parseUsersFromJson(const QByteArray &jsonData) {
    QList<User> users;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON parse error:" << parseError.errorString();
        return users;
    }

    if (doc.isObject()) {
        QJsonObject obj = doc.object();

        QString id = obj["id"].toString();
        QString password = obj["password"].toString();
        QString expiryStr = obj["expiryDate"].toString();

        //checking license expiration date
        QDateTime expiryDate = QDateTime::fromString(expiryStr, Qt::ISODate);
        QDateTime currentDateTime = QDateTime::currentDateTime();

        if (expiryDate.isValid() && currentDateTime > expiryDate) {
            qDebug() << "License expired on:" << expiryDate.toString(Qt::ISODate);
            return users; //returns empty list
        }

        if (!id.isEmpty()) {
            users.append(User(id, password));
        }
    }

    return users;
}