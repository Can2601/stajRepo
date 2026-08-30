#ifndef LOGINMANAGER_H
#define LOGINMANAGER_H

#include <QString>
#include <QList>
#include <QObject>
#include <QUrl>
#include "User.h"

class LoginManager : public QObject
{
    Q_OBJECT

public:
    LoginManager();

    Q_INVOKABLE bool loadLicense(const QUrl &fileUrl);
    Q_INVOKABLE bool login(const QString& id, const QString& password);

    Q_INVOKABLE QString getExpiryDate() const { return expiryDate; }

private:
    QList<User> users;
    QString expiryDate; //to keep the expiry date
};

#endif // LOGINMANAGER_H