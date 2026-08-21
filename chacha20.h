#ifndef CHACHA20_H
#define CHACHA20_H

#include <QByteArray>
#include <cstdint>
#include <cstring>

// ChaCha20 stream cipher — RFC 8439 (IETF) implementasyonu.
// Harici kütüphaneye (OpenSSL/libsodium) ihtiyaç duymadan çalışır.
// Bu dosya login uygulamasına da AYNEN kopyalanmalı — şifre çözerken
// aynı algoritma kullanılmak zorunda.
//
// key   : 32 byte (256 bit) — GİZLİ, iki uygulamada da aynı olmalı.
// nonce : 12 byte (96 bit)  — GİZLİ DEĞİL, her şifrelemede rastgele üretilir
//                             ve şifreli veriyle birlikte gönderilir.
//
// ChaCha20 simetriktir: encrypt ve decrypt AYNI fonksiyon (XOR mantığı).
namespace ChaCha20 {

inline uint32_t rotl(uint32_t x, int n)
{
    return (x << n) | (x >> (32 - n));
}

inline void quarterRound(uint32_t &a, uint32_t &b, uint32_t &c, uint32_t &d)
{
    a += b; d ^= a; d = rotl(d, 16);
    c += d; b ^= c; b = rotl(b, 12);
    a += b; d ^= a; d = rotl(d, 8);
    c += d; b ^= c; b = rotl(b, 7);
}

// Tek bir 64 baytlık "keystream" bloğu üretir.
inline void block(const uint8_t key[32], const uint8_t nonce[12], uint32_t counter, uint8_t out[64])
{
    uint32_t state[16] = {
        0x61707865, 0x3320646e, 0x79622d32, 0x6b206574, // "expand 32-byte k"
        0, 0, 0, 0, 0, 0, 0, 0,                          // key
        counter,                                         // block counter
        0, 0, 0                                          // nonce
    };
    memcpy(&state[4], key, 32);
    memcpy(&state[13], nonce, 12);

    uint32_t working[16];
    memcpy(working, state, sizeof(state));

    for (int i = 0; i < 10; ++i) {
        quarterRound(working[0], working[4], working[8],  working[12]);
        quarterRound(working[1], working[5], working[9],  working[13]);
        quarterRound(working[2], working[6], working[10], working[14]);
        quarterRound(working[3], working[7], working[11], working[15]);
        quarterRound(working[0], working[5], working[10], working[15]);
        quarterRound(working[1], working[6], working[11], working[12]);
        quarterRound(working[2], working[7], working[12], working[13]);
        quarterRound(working[3], working[4], working[9],  working[14]);
    }
    for (int i = 0; i < 16; ++i) {
        working[i] += state[i];
    }
    memcpy(out, working, 64);
}

// Veriyi key+nonce ile şifreler (ya da çözer — ikisi de aynı işlem).
inline QByteArray crypt(const QByteArray &input, const uint8_t key[32], const uint8_t nonce[12])
{
    QByteArray output(input.size(), Qt::Uninitialized);
    uint8_t keystream[64];
    uint32_t counter = 1; // RFC 8439'da blok sayacı 1'den başlar

    int offset = 0;
    while (offset < input.size()) {
        block(key, nonce, counter, keystream);
        int chunk = qMin(64, input.size() - offset);
        for (int i = 0; i < chunk; ++i) {
            output[offset + i] = input[offset + i] ^ keystream[i];
        }
        offset += chunk;
        counter++;
    }
    return output;
}

} // namespace ChaCha20

#endif // CHACHA20_H
