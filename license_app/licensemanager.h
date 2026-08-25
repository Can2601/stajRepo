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

// ── License file format ───────────────────────────────────────────────────────
// Each file written to SHARED_LICENSE_FOLDER/<id>.json contains:
//
//   {
//     "bundle": "<base64( nonce[24] | ciphertext | poly1305_tag[16] )>"
//   }
//
// The inner plaintext (before encryption) is compact JSON:
//   {"id":"...","password":"...","createdAt":"...","expiryDate":"..."}
//
// Algorithm : XChaCha20-Poly1305 (IETF) via libsodium
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

    // Called from QML: licenseManager.generateAndSave(365)
    // Returns a map with: id, password, expiryDate, filePath, success, error
    Q_INVOKABLE QVariantMap generateAndSave(int validDays = 365)
    {
        QVariantMap result;

        // 1. Generate license data
        QString id         = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QString password   = generatePassword();
        QDateTime created  = QDateTime::currentDateTime();
        QDateTime expiry   = created.addDays(validDays);

        // 2. Build compact JSON plaintext
        QJsonObject licenseJson;
        licenseJson["id"]          = id;
        licenseJson["password"]    = password;
        licenseJson["createdAt"]   = created.toString(Qt::ISODate);
        licenseJson["expiryDate"]  = expiry.toString(Qt::ISODate);
        QByteArray plaintext = QJsonDocument(licenseJson).toJson(QJsonDocument::Compact);

        // 3. Generate a random 24-byte nonce (XChaCha20 uses 24-byte nonces —
        //    large enough to be safe with randombytes_buf, no counter needed)
        unsigned char nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];
        randombytes_buf(nonce, sizeof(nonce));

        // 4. Encrypt with XChaCha20-Poly1305
        //    Output size = plaintext size + 16-byte Poly1305 tag
        size_t cipherLen = static_cast<size_t>(plaintext.size())
                           + crypto_aead_xchacha20poly1305_ietf_ABYTES;
        QByteArray cipherBuf(static_cast<int>(cipherLen), Qt::Uninitialized);
        unsigned long long actualLen = 0;

        int rc = crypto_aead_xchacha20poly1305_ietf_encrypt(
            reinterpret_cast<unsigned char*>(cipherBuf.data()), &actualLen,
            reinterpret_cast<const unsigned char*>(plaintext.constData()),
            static_cast<unsigned long long>(plaintext.size()),
            nullptr, 0,   // no additional data
            nullptr,      // nsec — not used
            nonce,
            LICENSE_KEY
        );

        if (rc != 0) {
            result["success"] = false;
            result["error"]   = "Encryption failed (libsodium error)";
            return result;
        }

        cipherBuf.resize(static_cast<int>(actualLen));

        // 5. Bundle: nonce | ciphertext+tag  →  base64
        QByteArray nonceBuf(reinterpret_cast<const char*>(nonce), sizeof(nonce));
        QByteArray bundle  = nonceBuf + cipherBuf;
        QString    b64     = QString::fromLatin1(bundle.toBase64());

        // 6. Write envelope JSON
        QJsonObject envelope;
        envelope["bundle"] = b64;

        QDir dir(SHARED_LICENSE_FOLDER);
        if (!dir.exists())
            dir.mkpath(".");

        QString filePath = dir.filePath(id + ".json");
        QFile   file(filePath);
        bool    success = false;
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(envelope).toJson(QJsonDocument::Indented));
            file.close();
            success = true;
        }

        result["success"]    = success;
        result["id"]         = id;
        result["password"]   = password;
        result["expiryDate"] = expiry.toString("dd.MM.yyyy");
        result["filePath"]   = filePath;
        if (!success)
            result["error"] = "Could not write license file to: " + filePath;

        return result;
    }

private:
    QString generatePassword(int length = 16)
    {
        const QString chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%";
        QString result;
        result.reserve(length);
        for (int i = 0; i < length; ++i) {
            // Use libsodium's CSPRNG for password generation too
            quint32 idx = randombytes_uniform(static_cast<uint32_t>(chars.length()));
            result.append(chars.at(static_cast<int>(idx)));
        }
        return result;
    }
};

#endif // LICENSEMANAGER_H
