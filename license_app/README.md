# License Generator

Lisans üretici Qt Quick uygulaması. ID + şifre + son kullanma tarihi üretir,
login uygulamasıyla paylaşılan bir klasöre **ChaCha20 ile şifreleyerek** yazar.

Şifresiz (plain text) sürüm için bkz. `unencrypted-license` branch'i.

## Çalıştırma

```
license_app.pro dosyasını Qt Creator'da aç → Run.
```

Arayüzdeki "Lisans Üret" butonu, `LicenseManager::generateAndSave(validDays)`
fonksiyonunu çağırır (bkz. `licensemanager.h`).

## Çıktı nereye yazılıyor?

```
%HOMEPATH%/LicenseShared/licenses/<id>.json
```

(`SHARED_LICENSE_FOLDER` sabiti, `licensemanager.h` içinde tanımlı.)

## JSON formatı

### Şifrelenmemiş (düz metin) — asıl lisans verisi bu

Şifreleme öncesi hazırlanan, dosyaya **yazılmayan** ara veri:

```json
{
    "createdAt": "2026-08-19T09:37:11",
    "expiryDate": "2027-08-19T09:37:11",
    "id": "c3f88da8-920d-4c34-b51b-c4dffb50f250",
    "password": "F1LvZeJHAyVLxTuO"
}
```

| Alan | Tip | Açıklama |
|---|---|---|
| `id` | string (UUID) | Lisansın benzersiz kimliği. Dosya adı da bu değer (`<id>.json`). |
| `password` | string | 16 karakter, rastgele üretilmiş lisans şifresi. |
| `createdAt` | string (ISO 8601) | Üretim tarihi/saati. |
| `expiryDate` | string (ISO 8601) | Son kullanma tarihi/saati (`generateAndSave`'e verilen `validDays` kadar sonrası). |

### Şifrelenmiş — dosyaya asıl yazılan format

Yukarıdaki JSON, ChaCha20 ile şifrelenip aşağıdaki "zarf" (envelope) formatında diske yazılır:

```json
{
    "nonce": "5f713bc2b4ab6abf6ba1d7f0",
    "cipher": "10fbfaac00f9b5d4662eb00d5351a4c8664e63c9..."
}
```

| Alan | Tip | Açıklama |
|---|---|---|
| `nonce` | string (hex, 24 karakter = 12 byte) | Her lisans için rastgele üretilir. Gizli değildir, şifre çözmek için gereklidir. |
| `cipher` | string (hex) | Yukarıdaki düz metin JSON'un ChaCha20 ile şifrelenmiş hali. |

## Şifrelemeyi hangi fonksiyon yapıyor

`chacha20.h` içindeki tek fonksiyon:

```cpp
QByteArray ChaCha20::crypt(const QByteArray &input, const uint8_t key[32], const uint8_t nonce[12]);
```

- **Şifrelerken** (`licensemanager.h` → `generateAndSave`):
  ```cpp
  QByteArray cipherBytes = ChaCha20::crypt(plainBytes, LICENSE_KEY, nonce);
  ```
- **Çözerken** (login uygulaması tarafında), **aynı fonksiyon aynı şekilde çağrılır** —
  ChaCha20 simetriktir, encrypt/decrypt için ayrı bir fonksiyon yoktur:
  ```cpp
  QByteArray plainBytes = ChaCha20::crypt(cipherBytes, LICENSE_KEY, nonceFromFile);
  ```

`plainBytes`, yukarıdaki "şifrelenmemiş" formattaki JSON'un ham (compact) bayt
karşılığıdır — `QJsonDocument::fromJson(plainBytes).object()` ile tekrar
`QJsonObject`'e çevrilip `id`/`password`/`expiryDate` okunabilir.

## Login uygulamasına gerekenler

1. `chacha20.h` dosyasını birebir kopyala.
2. `licensemanager.h` içindeki `LICENSE_KEY` dizisini (32 byte) birebir aynı kullan.
3. `SHARED_LICENSE_FOLDER` ile aynı klasörden dosyaları oku.
4. Kullanıcı girdiği ID ile `<id>.json` dosyasını aç, `nonce`/`cipher`'ı hex'ten
   `QByteArray`'e çevirip yukarıdaki `ChaCha20::crypt(...)` çağrısıyla çöz,
   çıkan JSON'daki `password`'ü kullanıcı girdisiyle, `expiryDate`'i de
   `QDateTime::currentDateTime()` ile karşılaştır.
