#ifndef LOGINMANAGER_H
#define LOGINMANAGER_H
#include <QString>
#include <QList>
#include <User.h>
#include <QObject>

//using QObject in order to use in QML

class LoginManager : public QObject
{
    Q_OBJECT

public:
    LoginManager();

    Q_INVOKABLE bool login(const QString& id, const QString& password);

private:
    QList<User> users;
};

#endif // LOGINMANAGER_H
