#ifndef LICENSEMANAGER_H
#define LICENSEMANAGER_H

#include <QObject>
#include <QVariantMap>
#include <QString>

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
extern const QString SHARED_LICENSE_FOLDER;

class LicenseManager : public QObject
{
    Q_OBJECT
public:
    explicit LicenseManager(QObject *parent = nullptr);

    // Function that generates a license using custom user inputs from the UI
    Q_INVOKABLE QVariantMap generateCustomLicense(QString id, QString password, QString expiryDateStr, QString customFolder = "");
};

#endif // LICENSEMANAGER_H