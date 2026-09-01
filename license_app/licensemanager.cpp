#include "licensemanager.h"
#include "shared_license_key.h"
#include "private_license_key.h"

#include <QUuid>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <sodium.h>

const QString SHARED_LICENSE_FOLDER = QDir::homePath() + "/LicenseShared/licenses";

LicenseManager::LicenseManager(QObject *parent) : QObject(parent)
{
    if (sodium_init() < 0) {
        qWarning("LicenseManager: libsodium initialization failed!");
    }
}

QVariantMap LicenseManager::generateCustomLicense(QString id, QString password, QString expiryDateStr, QString customFolder)
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

    // 1. Hash the Password with ARGON2
    QByteArray passBytes = password.toUtf8();
    char hashed_password_str[crypto_pwhash_STRBYTES];

    if (crypto_pwhash_str(
            hashed_password_str,
            passBytes.constData(),
            passBytes.size(),
            crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
        result["success"] = false;
        result["error"]   = "Memory allocation failed for Argon2!";
        return result;
    }

    QString hashedPassword = QString::fromUtf8(hashed_password_str);

    // 2. Build compact JSON plaintext
    QJsonObject licenseJson;
    licenseJson["id"]          = id;
    licenseJson["password"]    = hashedPassword; // Store the hash, NOT the plaintext password!
    licenseJson["createdAt"]   = created.toString(Qt::ISODate);
    licenseJson["expiryDate"]  = expiry.toString(Qt::ISODate);
    QByteArray rawJsonData = QJsonDocument(licenseJson).toJson(QJsonDocument::Compact);

    // 3. JSON Integrity Hash (Task: Hash the entire JSON content)
    unsigned char jsonHash[crypto_generichash_BYTES];
    crypto_generichash(jsonHash, sizeof(jsonHash),
                       reinterpret_cast<const unsigned char*>(rawJsonData.constData()),
                       rawJsonData.size(), nullptr, 0);

    // 4. Sign the Hash with Private Key (Task: Sign the hash with private key)
    QByteArray privateKey = PRIVATE_LICENSE_KEY;

    unsigned char signature[crypto_sign_BYTES];
    unsigned long long sigLen;

    // Sign only the hash of the JSON
    crypto_sign_detached(signature, &sigLen,
                         jsonHash, sizeof(jsonHash),
                         reinterpret_cast<const unsigned char*>(privateKey.constData()));

    QString signatureHex = QString(QByteArray(reinterpret_cast<char*>(signature), sigLen).toHex());

    // 5. Combine original data and signature into a final JSON package
    QJsonObject finalSignedJson;
    finalSignedJson["licenseData"] = licenseJson; // Login app reads this part
    finalSignedJson["signature"]   = signatureHex;  // Login app verifies this signature using the public key

    // The final plaintext to be encrypted (XChaCha20)
    QByteArray plaintext = QJsonDocument(finalSignedJson).toJson(QJsonDocument::Compact);

    // 6. Encrypt with XChaCha20-Poly1305 (Libsodium)
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

    // 7. Bundle (nonce + ciphertext)
    QByteArray nonceBuf(reinterpret_cast<const char*>(nonce), sizeof(nonce));
    QByteArray bundle  = nonceBuf + cipherBuf;

    // 8. Determine target directory
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

    // 9. Return results to QML
    result["success"]    = success;
    result["id"]         = id;
    result["password"]   = password;
    result["expiryDate"] = expiry.toString("dd.MM.yyyy");
    result["filePath"]   = filePath;
    if (!success) result["error"] = "Could not write license file to: " + filePath;

    return result;
}