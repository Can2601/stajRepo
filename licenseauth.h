#ifndef LICENSEAUTH_H
#define LICENSEAUTH_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

#include "crypto_utils.h"      // encrypt / decrypt / toBase64 — defined here
#include "shared_license_key.h" // LICENSE_KEY[32]

// ── LicenseAuth ───────────────────────────────────────────────────────────────
// Exposed to QML as "LicenseAuth" context property.
//
// QML usage:
//   var ok = LicenseAuth.validateCredentials(idField.text, passwordField.text)
//   if (ok) loginPage.loginSucceeded(id)
//   else    loginPage._errorText = LicenseAuth.lastError()
//
// License files are read from:
//   ~/LicenseShared/licenses/<id>.json
//
// Each file contains:
//   { "bundle": "<base64( nonce[24] | ciphertext | poly1305_tag[16] )>" }
//
// Decryption uses decrypt() from crypto_utils.h — the same function
// that lives in encrypt_demo.cpp.
// ─────────────────────────────────────────────────────────────────────────────

class LicenseAuth : public QObject
{
    Q_OBJECT
public:
    explicit LicenseAuth(QObject *parent = nullptr) : QObject(parent)
    {
        if (sodium_init() < 0)
            m_lastError = "libsodium initialization failed";
    }

    // Returns true if id + password match a valid, non-expired license file.
    Q_INVOKABLE bool validateCredentials(const QString &id, const QString &password)
    {
        m_lastError.clear();

        // 1. Locate the license file
        QString filePath = QDir::homePath()
                           + "/LicenseShared/licenses/"
                           + id + ".json";

        if (!QFile::exists(filePath)) {
            m_lastError = "License not found for this User ID.";
            return false;
        }

        // 2. Read envelope JSON
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            m_lastError = "Could not open license file.";
            return false;
        }
        QByteArray raw = file.readAll();
        file.close();

        QJsonParseError parseErr;
        QJsonDocument   doc = QJsonDocument::fromJson(raw, &parseErr);
        if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
            m_lastError = "License file is corrupted (invalid JSON).";
            return false;
        }

        QJsonObject envelope = doc.object();
        if (!envelope.contains("bundle") || !envelope["bundle"].isString()) {
            m_lastError = "License file is corrupted (missing bundle field).";
            return false;
        }

        // 3. Decrypt using the same decrypt() from crypto_utils.h
        std::string bundle    = envelope["bundle"].toString().toStdString();
        std::string plaintext;

        try {
            plaintext = decrypt(bundle, LICENSE_KEY); // ← same function as encrypt_demo
        } catch (const std::exception &e) {
            m_lastError = QString("Decryption failed: %1").arg(e.what());
            return false;
        }

        // 4. Parse the decrypted license JSON
        QJsonDocument licenseDoc = QJsonDocument::fromJson(
            QByteArray::fromStdString(plaintext), &parseErr);

        if (parseErr.error != QJsonParseError::NoError || !licenseDoc.isObject()) {
            m_lastError = "Decrypted license data is invalid.";
            return false;
        }

        QJsonObject license = licenseDoc.object();

        // 5. Check password
        if (license["password"].toString() != password) {
            m_lastError = "Incorrect password.";
            return false;
        }

        // 6. Check expiry
        QDateTime expiry = QDateTime::fromString(
            license["expiryDate"].toString(), Qt::ISODate);

        if (!expiry.isValid()) {
            m_lastError = "License expiry date is invalid.";
            return false;
        }
        if (QDateTime::currentDateTime() > expiry) {
            m_lastError = QString("License expired on %1.")
                              .arg(expiry.toString("dd.MM.yyyy"));
            return false;
        }

        return true;
    }

    // Human-readable reason for the last failure.
    Q_INVOKABLE QString lastError() const { return m_lastError; }

private:
    QString m_lastError;
};

#endif // LICENSEAUTH_H
