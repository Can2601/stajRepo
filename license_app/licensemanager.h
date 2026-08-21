#ifndef LICENSEMANAGER_H
#define LICENSEMANAGER_H

#include <QObject>
#include <QVariantMap>
#include <QUuid>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QRandomGenerator>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QStandardPaths>

// Login uygulamasıyla PAYLAŞILAN klasör.
// Login app da lisansları doğrularken tam olarak bu klasörden okumalı:
// Windows'ta genelde: C:/Users/<kullanıcı>/LicenseShared/licenses
// Bu yolu login uygulamasını yazan arkadaşınla paylaşman lazım.
static const QString SHARED_LICENSE_FOLDER =
    QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/LicenseShared/licenses";

// Bu sınıf QML tarafına köprü görevi görür.
// Q_INVOKABLE işaretli fonksiyonlar QML içinden çağrılabilir.
class LicenseManager : public QObject
{
    Q_OBJECT
public:
    explicit LicenseManager(QObject *parent = nullptr) : QObject(parent) {}

    // QML'den: licenseManager.generateAndSave(365) şeklinde çağrılır.
    // Geriye id/password/expiryDate/filePath bilgisini QML'in okuyabileceği bir map olarak döner.
    Q_INVOKABLE QVariantMap generateAndSave(int validDays = 365)
    {
        QString id = generateId();
        QString password = generatePassword();
        QDateTime createdAt = QDateTime::currentDateTime();
        QDateTime expiryDate = createdAt.addDays(validDays);

        // ŞİFRELEME YOK — bu branch bilinçli olarak açık (plain text) yazıyor.
        // Şifreli sürüm için "chacha20-encryption" branch'ine bak.
        QJsonObject license;
        license["id"] = id;
        license["password"] = password;
        license["createdAt"] = createdAt.toString(Qt::ISODate);
        license["expiryDate"] = expiryDate.toString(Qt::ISODate);

        // Kendi klasörümüz yerine login app ile PAYLAŞILAN klasöre yazıyoruz.
        QDir dir(SHARED_LICENSE_FOLDER);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        QString filePath = dir.filePath(id + ".json");

        QJsonDocument doc(license);
        QFile file(filePath);
        bool success = false;
        if (file.open(QIODevice::WriteOnly)) {
            file.write(doc.toJson(QJsonDocument::Indented));
            file.close();
            success = true;
        }

        QVariantMap result;
        result["id"] = id;
        result["password"] = password;
        result["expiryDate"] = expiryDate.toString("dd.MM.yyyy");
        result["filePath"] = filePath;
        result["success"] = success;
        return result;
    }

private:
    QString generateId()
    {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    QString generatePassword(int length = 16)
    {
        const QString chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%";
        QString result;
        for (int i = 0; i < length; ++i) {
            int index = QRandomGenerator::global()->bounded(chars.length());
            result.append(chars.at(index));
        }
        return result;
    }
};

#endif // LICENSEMANAGER_H
