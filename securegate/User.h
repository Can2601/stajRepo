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

    //setters
    void setID(const QString& id);
    void setPassword(const QString& password);

private:
    QString id;
    QString password;
};

#endif // USER_H
