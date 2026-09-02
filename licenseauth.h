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

#include "crypto_utils.h"        // decrypt() — XChaCha20-Poly1305
#include "shared_license_key.h"  // LICENSE_KEY[32]  — symmetric encryption key
#include "public_license_key.h"  // LICENSE_PUBLIC_KEY[32] — Ed25519 public key
#include "AppConfig.h"           // AppConfig::instance().licenseDir

// ── LicenseAuth ───────────────────────────────────────────────────────────────
// Exposed to QML as "LicenseAuth" context property.
//
// QML usage:
//   var ok = LicenseAuth.validateCredentials(idField.text, passwordField.text)
//   if (ok) loginPage.loginSucceeded(id)
//   else    loginPage._errorText = LicenseAuth.lastError()
//
// License files are read from:
//   <licenseDir>/<id>.lic   (raw binary format)
//
// .lic file layout (raw bytes, no Base64):
//   [ Nonce (24 B) | Ciphertext | Poly1305 Tag (16 B) ]
//
// Decrypted plaintext is a compact JSON signed envelope:
//   {
//     "licenseData": {
//       "id":         "<user_id>",
//       "password":   "<argon2id_hash_string>",
//       "createdAt":  "<ISO 8601>",
//       "expiryDate": "<ISO 8601>"
//     },
//     "signature": "<ed25519_signature_hex>"
//   }
//
// Validation pipeline (all steps must pass):
//   1. Locate .lic file
//   2. Read raw binary bundle
//   3. Decrypt with XChaCha20-Poly1305 (tamper → exception)
//   4. Parse outer signed JSON envelope
//   5. Re-serialise licenseData → BLAKE2b hash
//   6. Verify Ed25519 signature against hash using LICENSE_PUBLIC_KEY
//   7. Verify password with Argon2id (crypto_pwhash_str_verify)
//   8. Check expiry date
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

    // Returns true if id + password match a valid, non-expired, untampered license.
    Q_INVOKABLE bool validateCredentials(const QString &id, const QString &password)
    {
        m_lastError.clear();

        QString lDir = AppConfig::instance().licenseDir;

        // 1. Locate the .lic file
        QString filePath = lDir + "/" + id + ".lic";

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

        // 3. Decrypt with XChaCha20-Poly1305
        //    Any tampering with the encrypted bundle is caught here by the Poly1305 MAC.
        std::string plaintext;
        try {
            plaintext = decrypt(bundle, LICENSE_KEY);
        } catch (const std::exception &e) {
            m_lastError = QString("Decryption failed: %1").arg(e.what());
            return false;
        }

        // 4. Parse outer signed JSON envelope
        QJsonParseError parseErr;
        QJsonDocument outerDoc = QJsonDocument::fromJson(
            QByteArray::fromStdString(plaintext), &parseErr);

        if (parseErr.error != QJsonParseError::NoError || !outerDoc.isObject()) {
            m_lastError = "Decrypted license data is invalid JSON.";
            return false;
        }

        QJsonObject outer = outerDoc.object();
        if (!outer.contains("licenseData") || !outer.contains("signature")) {
            m_lastError = "License format invalid (missing licenseData or signature).";
            return false;
        }

        QJsonObject licenseData = outer["licenseData"].toObject();
        QString     signatureHex = outer["signature"].toString();

        // 5. Re-serialise licenseData to compact JSON and BLAKE2b-hash it.
        //    This MUST produce byte-for-byte the same output as the license app did
        //    before signing (QJsonDocument::Compact, same key insertion order).
        QByteArray rawJsonData =
            QJsonDocument(licenseData).toJson(QJsonDocument::Compact);

        unsigned char jsonHash[crypto_generichash_BYTES];
        crypto_generichash(jsonHash, sizeof(jsonHash),
                           reinterpret_cast<const unsigned char*>(rawJsonData.constData()),
                           static_cast<unsigned long long>(rawJsonData.size()),
                           nullptr, 0);

        // 6. Verify Ed25519 signature against the hash using the hardcoded public key.
        //    A valid signature proves the file was produced by the genuine license app.
        QByteArray sigBytes = QByteArray::fromHex(signatureHex.toUtf8());
        if (static_cast<size_t>(sigBytes.size()) != crypto_sign_BYTES) {
            m_lastError = "License signature has invalid length.";
            return false;
        }

        if (crypto_sign_verify_detached(
                reinterpret_cast<const unsigned char*>(sigBytes.constData()),
                jsonHash, sizeof(jsonHash),
                LICENSE_PUBLIC_KEY) != 0) {
            m_lastError = "License signature verification failed. File may be tampered.";
            return false;
        }

        // 7. Verify password with Argon2id (crypto_pwhash_str_verify).
        //    The license file stores the Argon2id hash string from crypto_pwhash_str().
        //    This function re-derives and compares in constant time — no plaintext stored.
        QString    storedHash      = licenseData["password"].toString();
        QByteArray storedHashBytes = storedHash.toUtf8();
        QByteArray passwordBytes   = password.toUtf8();

        // Copy into a fixed-size buffer (crypto_pwhash_STRBYTES = 128)
        char hashBuf[crypto_pwhash_STRBYTES] = {};
        qstrncpy(hashBuf, storedHashBytes.constData(), crypto_pwhash_STRBYTES);

        if (crypto_pwhash_str_verify(
                hashBuf,
                passwordBytes.constData(),
                static_cast<unsigned long long>(passwordBytes.size())) != 0) {
            m_lastError = "Incorrect password.";
            return false;
        }

        // 8. Check expiry date
        QDateTime expiry = QDateTime::fromString(
            licenseData["expiryDate"].toString(), Qt::ISODate);

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
