#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <sodium.h>
#include <iostream>
#include <string>
#include <vector>
#include <FileReader.h>
#include <QDebug>
#include <loginmanager.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
/*
    //libsodium control
    if (sodium_init() < 0)
    {
        std::cout << "libsodium initalization failed." << std::endl;
        return -1;
    }
    std::cout << "libsodium initalization is successful." << std::endl;


    //key creation
    static const unsigned char LICENSE_KEY[crypto_aead_xchacha20poly1305_ietf_KEYBYTES] = {
        0x2b, 0x7e, 0x15, 0x16,  0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88,  0x09, 0xcf, 0x4f, 0x3c,
        0x00, 0x11, 0x22, 0x33,  0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb,  0xcc, 0xdd, 0xee, 0xff
    };


    //nonce creation
    unsigned char nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];
    randombytes_buf(nonce, sizeof(nonce));


    //message to encrypt
    std::string message = "this is a secret message";


    //encryption
    std::vector<unsigned char> ciphertext(message.size() + crypto_aead_xchacha20poly1305_ietf_ABYTES);
    unsigned long long ciphertext_len = 0;

    int result = crypto_aead_xchacha20poly1305_ietf_encrypt(ciphertext.data(),
                                                  &ciphertext_len,
                                                  reinterpret_cast<const unsigned char*>(message.data()),
                                                  message.size(),
                                                  nullptr,
                                                  0,
                                                  nullptr,
                                                  nonce, LICENSE_KEY);
    if (result != 0){
        std::cout << "encryption failed." << std::endl;
        return 1;
    }

    std::cout << "encryption successful." << std::endl;

    //packing (nonce + ciphertext = packet)
    std::vector<unsigned char> packet(crypto_aead_xchacha20poly1305_ietf_NPUBBYTES + ciphertext_len);
    memcpy(packet.data(), nonce, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
    memcpy(packet.data() + crypto_aead_xchacha20poly1305_ietf_NPUBBYTES, ciphertext.data(), ciphertext_len);

    std::cout << "packet created successfully." << std::endl;
    std::cout << "packet size: " << packet.size() << " bytes." <<std::endl;


    //unpacking (packet = nonce + ciphertext)
    unsigned char received_nonce[crypto_aead_xchacha20poly1305_ietf_NPUBBYTES];
    memcpy(received_nonce, packet.data(), crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);

    unsigned char* received_ciphertext = packet.data() + crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
    unsigned long long received_ciphertext_len = packet.size() - crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;

    std::cout << "packet unpacked successfully." << std::endl;


    //decryption
    unsigned char decrypted[1024];
    unsigned long long decrypted_len = 0;

    int decrypted_result = crypto_aead_xchacha20poly1305_ietf_decrypt(decrypted,
                                                                     &decrypted_len,
                                                                     nullptr,
                                                                     received_ciphertext,
                                                                     received_ciphertext_len,
                                                                     nullptr,
                                                                     0,
                                                                     received_nonce,
                                                                     LICENSE_KEY);

    if(decrypted_result != 0){
            std::cout << "decryption failed." << std::endl;
            return 1;
        }

    std::cout << "decryption successful." << std::endl;

    std::string decrypted_message(reinterpret_cast<char*>(decrypted), decrypted_len);
    std::cout << "decrypted message: " << decrypted_message << std::endl;
*/

    //login manager definition
    LoginManager loginManager;


    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("loginManager", &loginManager);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("securegate", "Main");

    return QGuiApplication::exec();
}
