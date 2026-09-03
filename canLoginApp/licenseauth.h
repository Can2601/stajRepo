#ifndef LICENSEAUTH_H
#define LICENSEAUTH_H

#include <QObject>
#include <QString>

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
    explicit LicenseAuth(QObject *parent = nullptr);

    // Returns true if id + password match a valid, non-expired, untampered license.
    Q_INVOKABLE bool validateCredentials(const QString &id, const QString &password);

    // Human-readable reason for the last failure.
    Q_INVOKABLE QString lastError() const;

private:
    QString m_lastError;
};

#endif // LICENSEAUTH_H
