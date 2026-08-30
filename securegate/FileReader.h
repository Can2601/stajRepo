#ifndef FILEREADER_H
#define FILEREADER_H

#include "User.h"
#include <QList>
#include <QString>
#include <QByteArray>

class FileReader
{
public:
    FileReader();

    // Şifreli lisans dosyasını diskten ham byte (binary) olarak okur
    QByteArray readEncryptedFile(const QString &filePath);

    // Decrypt edilmiş JSON verisini QList<User> nesnesine çevirir
    QList<User> parseUsersFromJson(const QByteArray &jsonData);
};

#endif // FILEREADER_H