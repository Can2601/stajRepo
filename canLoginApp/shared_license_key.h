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
static const unsigned char LICENSE_KEY[crypto_aead_xchacha20poly1305_ietf_KEYBYTES] = {
    0x2b, 0x7e, 0x15, 0x16,  0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88,  0x09, 0xcf, 0x4f, 0x3c,
    0x00, 0x11, 0x22, 0x33,  0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb,  0xcc, 0xdd, 0xee, 0xff
};

#endif // SHARED_LICENSE_KEY_H
