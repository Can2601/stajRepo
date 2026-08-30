#ifndef USER_H
#define USER_H
#include <QString>

class User
{

public:
    //constructor
    User(const QString& id, const QString& password);

    //getters
    QString getID() const;
    QString getPassword() const;

private:
    QString id;
    QString password;
};

#endif // USER_H
