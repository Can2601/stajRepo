// =========================================================
//  crypto_utils.h  –  libsodium ChaCha20-Poly1305 utilities
//
//  Uses: crypto_aead_xchacha20poly1305_ietf_encrypt/decrypt
//        Algorithm : XChaCha20-Poly1305 (IETF)
//        Key       : 32 bytes
//        Nonce     : 24 bytes (random, prepended to bundle)
//        MAC tag   : 16 bytes (Poly1305 – tamper detection)
// =========================================================

#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <sodium.h>
#include <string>
#include <vector>
#include <QByteArray>

// Short aliases for the IETF XChaCha20-Poly1305 constants
constexpr size_t KEY_BYTES   = crypto_aead_xchacha20poly1305_ietf_KEYBYTES;   // 32
constexpr size_t NONCE_BYTES = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;  // 24
constexpr size_t MAC_BYTES   = crypto_aead_xchacha20poly1305_ietf_ABYTES;     // 16


// ----------------------------------------------------------
// Helper: raw bytes → Base64 string (sodium built-in)
// ----------------------------------------------------------
std::string toBase64(const unsigned char* data, size_t len);

// ----------------------------------------------------------
// ENCRYPT  (XChaCha20-Poly1305)
//
//  Bundle layout stored in Base64:
//    [ nonce (24 B) | ciphertext (N B) | MAC tag (16 B) ]
//
//  Additional data (AD) is optional authenticated plaintext
//  that is NOT encrypted but IS covered by the MAC.
//  We use the app name as AD so a license file for one app
//  cannot be replayed against another.
// ----------------------------------------------------------
std::string encrypt(const std::string& plaintext,
                    const unsigned char key[KEY_BYTES],
                    const std::string& additionalData = "");


// ----------------------------------------------------------
// DECRYPT from raw binary bundle  (new .lic file format)
//
// Bundle layout — raw bytes, no base64:
//   [ nonce (24 B) | ciphertext (N B) | MAC tag (16 B) ]
//
// This is the binary that licensemanager writes directly
// to disk with QIODevice::WriteOnly.
// ----------------------------------------------------------
std::string decrypt(const QByteArray& bundle,
                    const unsigned char key[KEY_BYTES],
                    const std::string& additionalData = "");

#endif // CRYPTO_UTILS_H
