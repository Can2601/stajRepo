#include "User.h"

User::User(const QString& id, const QString& passwordHash) {
    this->id = id;
    this->passwordHash = passwordHash;
}

QString User::getID() const{
    return id;
}

QString User::getPasswordHash() const{
    return passwordHash;
}
