#ifndef LICENSEAUTH_H
#define LICENSEAUTH_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QByteArray>

#include "crypto_utils.h"      // encrypt / decrypt — defined here
#include "shared_license_key.h" // LICENSE_KEY[32]
#include "AppConfig.h"          // AppConfig::instance().licenseDir

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
        
        QString lDir = AppConfig::instance().licenseDir;

        // 1. Locate the .lic file (raw binary format)
        QString filePath =lDir + "/"+ id + ".lic";

        if (!QFile::exists(filePath)) {
            m_lastError = "License not found for this User ID.";
            return false;
        }

        // 2. Read raw binary bundle: nonce[24] | ciphertext | tag[16]
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            m_lastError = "Could not open license file.";
            return false;
        }
        QByteArray bundle = file.readAll();
        file.close();

        // 3. Decrypt
        std::string plaintext;
        try {
            plaintext = decrypt(bundle, LICENSE_KEY);
        } catch (const std::exception &e) {
            m_lastError = QString("Decryption failed: %1").arg(e.what());
            return false;
        }

        // 4. Parse the decrypted license JSON
        QJsonParseError parseErr;
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
