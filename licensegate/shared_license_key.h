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
    0x04, 0xae, 0xf5, 0xed, 0x1c, 0x28, 0xcb, 0x68, 
    0x00, 0xe2, 0xf5, 0x19, 0x15, 0x23, 0x27, 0xbc,
    0x5d, 0xd5, 0x4c, 0xc4, 0x7f, 0xf7, 0x6e, 0xe6,
    0x19, 0x91, 0x08, 0x80, 0x3b, 0xb3, 0x2a, 0xa2
};

static void deobfuscate(const unsigned char obfuscated[32], unsigned char out[32])
{
    unsigned char temp[32];

    for(int i=0; i<32; i++)
    {
        temp[i] = (obfuscated[i] >> 3) | (obfuscated[i] << 5);
    }

    for(int i=0; i<32; i++)
    {
        out[i] = temp[i] ^ 0xab;
    }
    
}
#endif // SHARED_LICENSE_KEY_H
