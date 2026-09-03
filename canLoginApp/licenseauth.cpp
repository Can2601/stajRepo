#include "licenseauth.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QByteArray>

#include "crypto_utils.h"
#include "shared_license_key.h"
#include "public_license_key.h"
#include "AppConfig.h"

LicenseAuth::LicenseAuth(QObject *parent) : QObject(parent)
{
    if (sodium_init() < 0)
        m_lastError = "libsodium initialization failed";
}

bool LicenseAuth::validateCredentials(const QString &id, const QString &password)
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

QString LicenseAuth::lastError() const { return m_lastError; }
