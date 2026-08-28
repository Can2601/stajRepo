#ifndef LICENSEMANAGER_H
#define LICENSEMANAGER_H

#include <QObject>
#include <QVariantMap>
#include <QUuid>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

#include <sodium.h>
#include "shared_license_key.h"

// ── License file format (Raw Binary) ───────────────────────────────────────────────────────
// Each file written to SHARED_LICENSE_FOLDER/<id>.lic contains raw bytes:
//
//   [ Nonce (24 bytes) ] + [ Ciphertext ] + [ Poly1305 Tag (16 bytes) ]
//
// The inner plaintext (before encryption) is compact JSON:
//   {"id":"...","password":"...","createdAt":"...","expiryDate":"..."}
//
// Algorithm : XChaCha20-Poly1305 (IETF) via libsodium
// Format    : Raw Binary (No Base64, No JSON wrapping)
// Nonce     : 24 bytes, randomly generated per license, NOT secret
// Key       : 32 bytes, shared secret — see shared_license_key.h
// Tag       : 16 bytes Poly1305 MAC appended by libsodium, detects tampering
// ─────────────────────────────────────────────────────────────────────────────

// Folder shared with the login app.
// Login app must read license files from this exact path.
// Windows default: C:/Users/<user>/LicenseShared/licenses
static const QString SHARED_LICENSE_FOLDER =
    QDir::homePath() + "/LicenseShared/licenses";

class LicenseManager : public QObject
{
    Q_OBJECT
public:
    explicit LicenseManager(QObject *parent = nullptr) : QObject(parent)
    {
        if (sodium_init() < 0) {
            qWarning("LicenseManager: libsodium initialization failed!");
        }
    }

    // Function that generates a license using custom user inputs from the UI
    Q_INVOKABLE QVariantMap generateCustomLicense(QString id, QString password, QString expiryDateStr, QString customFolder = "")
    {
        QVariantMap result;
        QDateTime created = QDateTime::currentDateTime();

        // Convert the string from the UI (format: "dd.MM.yyyy") to QDateTime
        QDateTime expiry = QDateTime::fromString(expiryDateStr, "dd.MM.yyyy");

        if (!expiry.isValid()) {
            result["success"] = false;
            result["error"] = "Invalid date format. Expected: dd.MM.yyyy";
            return result;
        }

        // 1. Build compact JSON plaintext
        QJsonObject licenseJson;
        licenseJson["id"]          = id;
        licenseJson["password"]    = password;
        licenseJson["createdAt"]   = created.toString(Qt::ISODate);
        licenseJson["expiryDate"]  = expiry.toString(Qt::ISODate);
        QByteArray plaintext = QJsonDocument(licenseJson).toJson(QJsonDocument::Compact);

        // 2. Encrypt with XChaCha20-Poly1305 (Libsodium)
        unsigned char nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];
        randombytes_buf(nonce, sizeof(nonce));

        size_t cipherLen = static_cast<size_t>(plaintext.size())
                           + crypto_aead_xchacha20poly1305_ietf_ABYTES;
        QByteArray cipherBuf(static_cast<int>(cipherLen), Qt::Uninitialized);
        unsigned long long actualLen = 0;

        int rc = crypto_aead_xchacha20poly1305_ietf_encrypt(
            reinterpret_cast<unsigned char*>(cipherBuf.data()), &actualLen,
            reinterpret_cast<const unsigned char*>(plaintext.constData()),
            static_cast<unsigned long long>(plaintext.size()),
            nullptr, 0, nullptr, nonce, LICENSE_KEY
            );

        if (rc != 0) {
            result["success"] = false;
            result["error"]   = "Encryption failed (libsodium error)";
            return result;
        }
        cipherBuf.resize(static_cast<int>(actualLen));

        // 3. Bundle (nonce + ciphertext)
        QByteArray nonceBuf(reinterpret_cast<const char*>(nonce), sizeof(nonce));
        QByteArray bundle  = nonceBuf + cipherBuf;

        // 4. Determine target directory
        QString targetFolderPath = customFolder.trimmed().isEmpty() ? SHARED_LICENSE_FOLDER : customFolder;

        if (targetFolderPath.startsWith("file:///")) {
#ifdef Q_OS_WIN
            targetFolderPath = targetFolderPath.mid(8); // Windows needs C:/...
#else
            targetFolderPath = targetFolderPath.mid(7); // Unix needs /...
#endif
        } else if (targetFolderPath.startsWith("file://")) {
            targetFolderPath = targetFolderPath.mid(7);
        }

        QDir dir(targetFolderPath);
        if (!dir.exists()) dir.mkpath(".");

        // Changed file extension to .lic to represent a raw license file
        QString filePath = dir.filePath(id + ".lic");
        QFile file(filePath);
        bool success = false;

        // QIODevice::WriteOnly writes the data in its raw binary format
        if (file.open(QIODevice::WriteOnly)) {
            file.write(bundle);
            file.close();
            success = true;
        }

        // 5. Return results to QML
        result["success"]    = success;
        result["id"]         = id;
        result["password"]   = password;
        result["expiryDate"] = expiry.toString("dd.MM.yyyy");
        result["filePath"]   = filePath;
        if (!success) result["error"] = "Could not write license file to: " + filePath;

        return result;
    }
};

#endif // LICENSEMANAGER_H
