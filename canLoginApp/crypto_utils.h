// =========================================================
//  crypto_utils.h  –  libsodium ChaCha20-Poly1305 utilities
//
//  Uses: crypto_aead_xchacha20poly1305_ietf_encrypt/decrypt
//        Algorithm : XChaCha20-Poly1305 (IETF)
//        Key       : 32 bytes
//        Nonce     : 24 bytes (random, prepended to bundle)
//        MAC tag   : 16 bytes (Poly1305 – tamper detection)
//
//  All functions are static (internal linkage) — safe to
//  include in multiple translation units without ODR issues.
// =========================================================

#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <sodium.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <QByteArray>

// Short aliases for the IETF XChaCha20-Poly1305 constants
static constexpr size_t KEY_BYTES   = crypto_aead_xchacha20poly1305_ietf_KEYBYTES;   // 32
static constexpr size_t NONCE_BYTES = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;  // 24
static constexpr size_t MAC_BYTES   = crypto_aead_xchacha20poly1305_ietf_ABYTES;     // 16


// ----------------------------------------------------------
// Helper: raw bytes → Base64 string (sodium built-in)
// ----------------------------------------------------------
static std::string toBase64(const unsigned char* data, size_t len)
{
    size_t b64len = sodium_base64_encoded_len(len, sodium_base64_VARIANT_ORIGINAL);
    std::string out(b64len, '\0');
    sodium_bin2base64(&out[0], b64len, data, len, sodium_base64_VARIANT_ORIGINAL);
    if (!out.empty() && out.back() == '\0') out.pop_back();
    return out;
}

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
static std::string encrypt(const std::string& plaintext,
                    const unsigned char key[KEY_BYTES],
                    const std::string& additionalData = "")
{
    // 1. Random 24-byte nonce
    unsigned char nonce[NONCE_BYTES];
    randombytes_buf(nonce, NONCE_BYTES);

    // 2. Output buffer: ciphertext + 16-byte MAC tag
    size_t cipherLen = plaintext.size() + MAC_BYTES;
    std::vector<unsigned char> cipher(cipherLen);
    unsigned long long actualLen = 0;

    // 3. Encrypt + authenticate
    crypto_aead_xchacha20poly1305_ietf_encrypt(
        cipher.data(), &actualLen,
        reinterpret_cast<const unsigned char*>(plaintext.data()), plaintext.size(),
        reinterpret_cast<const unsigned char*>(additionalData.data()), additionalData.size(),
        nullptr,   // nsec – not used
        nonce, key
    );

    // 4. Bundle: [nonce | cipher+tag]
    std::vector<unsigned char> bundle;
    bundle.insert(bundle.end(), nonce, nonce + NONCE_BYTES);
    bundle.insert(bundle.end(), cipher.begin(), cipher.begin() + actualLen);

    return toBase64(bundle.data(), bundle.size());
}


// ----------------------------------------------------------
// DECRYPT from raw binary bundle  (new .lic file format)
//
// Bundle layout — raw bytes, no base64:
//   [ nonce (24 B) | ciphertext (N B) | MAC tag (16 B) ]
//
// This is the binary that licensemanager writes directly
// to disk with QIODevice::WriteOnly.
// ----------------------------------------------------------
static std::string decrypt(const QByteArray& bundle,
                                  const unsigned char key[KEY_BYTES],
                                  const std::string& additionalData = "")
{
    if (static_cast<size_t>(bundle.size()) <= NONCE_BYTES + MAC_BYTES)
        throw std::runtime_error("Bundle too short");

    const auto* data = reinterpret_cast<const unsigned char*>(bundle.constData());

    // 1. Split nonce | cipher+tag
    unsigned char nonce[NONCE_BYTES];
    std::memcpy(nonce, data, NONCE_BYTES);

    const unsigned char* cipher    = data + NONCE_BYTES;
    size_t               cipherLen = static_cast<size_t>(bundle.size()) - NONCE_BYTES;
    size_t               plainLen  = cipherLen - MAC_BYTES;

    // 2. Decrypt + verify Poly1305 MAC
    std::vector<unsigned char> plain(plainLen);
    unsigned long long outLen = 0;

    if (crypto_aead_xchacha20poly1305_ietf_decrypt(
            plain.data(), &outLen,
            nullptr,
            cipher, cipherLen,
            reinterpret_cast<const unsigned char*>(additionalData.data()), additionalData.size(),
            nonce, key) != 0)
        throw std::runtime_error("Decryption failed: message tampered or wrong key!");

    return std::string(reinterpret_cast<char*>(plain.data()), outLen);
}

#endif // CRYPTO_UTILS_H
