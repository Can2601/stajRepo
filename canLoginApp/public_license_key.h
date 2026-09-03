#ifndef PUBLIC_LICENSE_KEY_H
#define PUBLIC_LICENSE_KEY_H

#include <sodium.h>

// ── Ed25519 Public Key (32 bytes) ─────────────────────────────────────────────
// Derived from the license app's secret key:
//   last 32 bytes of the 64-byte Ed25519 SK = [seed (32B) | public_key (32B)]
//
// Used by the login app to verify that each .lic file was signed by
// the genuine license app — detects tampering without holding the private key.
//
// NEVER put the private key here. Only this public key belongs in the login app.
// ─────────────────────────────────────────────────────────────────────────────
static const unsigned char LICENSE_PUBLIC_KEY[crypto_sign_PUBLICKEYBYTES] = {
    0x9c, 0xe4, 0x21, 0xd7, 0x15, 0x6e, 0x34, 0x8a,
    0x08, 0x64, 0xad, 0x24, 0x09, 0x13, 0xe6, 0x5e,
    0x6b, 0x06, 0xb5, 0x1d, 0xea, 0xc9, 0x82, 0x0f,
    0xea, 0xf2, 0x41, 0xf6, 0x90, 0xfc, 0x98, 0xf0
};

#endif // PUBLIC_LICENSE_KEY_H
