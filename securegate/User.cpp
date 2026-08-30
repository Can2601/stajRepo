#include "User.h"

User::User(const QString& id, const QString& password) {
    this->id = id;
    this->password = password;
}

QString User::getID() const{
    return id;
}

QString User::getPassword() const{
    return password;
}
