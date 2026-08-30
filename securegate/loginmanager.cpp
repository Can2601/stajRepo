#include "loginmanager.h"
#include "FileReader.h"
#include <sodium.h>
#include <QDebug>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

//32-byte XChaCha20-Poly1305 KEY
static const unsigned char LICENSE_KEY[crypto_aead_xchacha20poly1305_ietf_KEYBYTES] = {
    0x2b, 0x7e, 0x15, 0x16,  0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88,  0x09, 0xcf, 0x4f, 0x3c,
    0x00, 0x11, 0x22, 0x33,  0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb,  0xcc, 0xdd, 0xee, 0xff
};

//libsodium control
LoginManager::LoginManager() : QObject() {
    if (sodium_init() < 0) {
        qDebug() << "libsodium initalization failed.";
    }
}

bool LoginManager::loadLicense(const QUrl &fileUrl) {
    //the "file:///C:/..." file coming from QML is translated into a suitable format
    QString filePath = fileUrl.toLocalFile();
    if (filePath.isEmpty()) {
        filePath = fileUrl.toString(); //fallback
    }

    //read the file as encrypted
    FileReader reader;
    QByteArray packet = reader.readEncryptedFile(filePath);

    if (packet.isEmpty()) {
        qDebug() << "license file could NOT be read.";
        return false;
    }

    //size control
    const size_t min_len = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES +
                           crypto_aead_xchacha20poly1305_ietf_ABYTES;

    if (static_cast<size_t>(packet.size()) < min_len) {
        qDebug() << "invalid license size.";
        return false;
    }

    //unpacking packet = (nonce + ciphertext)
    //nonce (first 24 bytes)
    const unsigned char* received_nonce = reinterpret_cast<const unsigned char*>(packet.constData());

    //ciphertext
    const unsigned char* received_ciphertext = reinterpret_cast<const unsigned char*>(
        packet.constData() + crypto_aead_xchacha20poly1305_ietf_NPUBBYTES
        );

    unsigned long long received_ciphertext_len = packet.size() - crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;

    //decryption
    QByteArray decryptedJson;
    decryptedJson.resize(received_ciphertext_len - crypto_aead_xchacha20poly1305_ietf_ABYTES);

    unsigned long long decrypted_len = 0;

    int result = crypto_aead_xchacha20poly1305_ietf_decrypt(
        reinterpret_cast<unsigned char*>(decryptedJson.data()),
        &decrypted_len,
        nullptr,
        received_ciphertext,
        received_ciphertext_len,
        nullptr,
        0,
        received_nonce,
        LICENSE_KEY
        );

    if (result != 0) {
        qDebug() << "encryption failed.";
        return false;
    }

    decryptedJson.resize(decrypted_len);

    QJsonDocument doc = QJsonDocument::fromJson(decryptedJson);
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        expiryDate = obj["expiryDate"].toString(); // Tarihi değişkene kaydettik
    }

    //JSON to QList<User>
    users = reader.parseUsersFromJson(decryptedJson);

    if (users.isEmpty()) {
        qDebug() << "could NOT find user info.";
        return false;
    }

    qDebug() << "license has been uploaded.";
    return true;
}

bool LoginManager::login(const QString& id, const QString& password) {
    for (const User& user : users) {
        if (user.getID() == id && user.getPassword() == password) {
            return true;
        }
    }
    return false;
}