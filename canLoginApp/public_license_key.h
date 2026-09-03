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
extern const unsigned char LICENSE_PUBLIC_KEY[crypto_sign_PUBLICKEYBYTES];

#endif // PUBLIC_LICENSE_KEY_H
