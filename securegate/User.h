#ifndef USER_H
#define USER_H
#include <QString>

class User
{

public:
    //constructor
    User(const QString& id, const QString& passwordHash);

    //getters
    QString getID() const;
    QString getPasswordHash() const;

private:
    QString id;
    QString passwordHash;
};

#endif // USER_H
