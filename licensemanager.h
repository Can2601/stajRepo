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

#include "chacha20.h"

// ── ChaCha20 GİZLİ ANAHTARI ──────────────────────────────────────────
// Bu anahtar login uygulamasında BİREBİR AYNI olmalı, yoksa şifre çözülemez.
// Şu aşamada öğrenme amaçlı kod içine sabit yazıyoruz.
// 32 byte (256 bit) olmak zorunda.
static const uint8_t LICENSE_KEY[32] = {
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
};

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

        // 1) Önce normal (düz metin) lisans verisini JSON olarak hazırla.
        QJsonObject license;
        license["id"] = id;
        license["password"] = password;
        license["createdAt"] = createdAt.toString(Qt::ISODate);
        license["expiryDate"] = expiryDate.toString(Qt::ISODate);
        QByteArray plainBytes = QJsonDocument(license).toJson(QJsonDocument::Compact);

        // 2) Her lisans için RASTGELE bir nonce üret (gizli değil, sadece tek kullanımlık).
        //    Aynı nonce+key ile iki farklı veri asla şifrelenmemeli.
        uint8_t nonce[12];
        for (int i = 0; i < 12; ++i) {
            nonce[i] = static_cast<uint8_t>(QRandomGenerator::global()->bounded(256));
        }

        // 3) ChaCha20 ile şifrele.
        QByteArray cipherBytes = ChaCha20::crypt(plainBytes, LICENSE_KEY, nonce);

        // 4) Şifreli veriyi + nonce'u hex olarak sarmalayan dış JSON'u oluştur.
        //    Login uygulaması: nonce'u ve cipher'ı hex'ten çözüp,
        //    AYNI ChaCha20::crypt fonksiyonuyla (aynı key) tekrar XOR'layarak
        //    orijinal düz metni geri elde eder.
        QJsonObject envelope;
        envelope["nonce"] = QString(QByteArray(reinterpret_cast<const char*>(nonce), 12).toHex());
        envelope["cipher"] = QString(cipherBytes.toHex());

        // Kendi klasörümüz yerine login app ile PAYLAŞILAN klasöre yazıyoruz.
        QDir dir(SHARED_LICENSE_FOLDER);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        QString filePath = dir.filePath(id + ".json");

        QJsonDocument doc(envelope);
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
