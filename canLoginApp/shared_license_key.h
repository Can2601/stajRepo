#ifndef SHARED_LICENSE_KEY_H
#define SHARED_LICENSE_KEY_H

#include <sodium.h>

// ── SHARED ChaCha20-Poly1305 SECRET KEY ──────────────────────────────────────
// This 32-byte key MUST be byte-for-byte identical in both:
//   - license_app  (generates encrypted license files)
//   - login app    (decrypts and validates license files)
//
// Algorithm : XChaCha20-Poly1305 (IETF) via libsodium
// Key size  : 32 bytes (256 bit)
//
// In production this would be obfuscated / loaded from a secure store.
// For learning purposes it is hardcoded here.
// ─────────────────────────────────────────────────────────────────────────────
extern const unsigned char LICENSE_KEY[crypto_aead_xchacha20poly1305_ietf_KEYBYTES];

#endif // SHARED_LICENSE_KEY_H
