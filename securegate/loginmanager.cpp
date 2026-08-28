#include "loginmanager.h"
#include <FileReader.h>

LoginManager::LoginManager():QObject(){
    FileReader reader;
    users = reader.readUsers();
}

bool LoginManager::login(const QString& id, const QString& password){
    for(const User& user : users){
        if(user.getID() == id && user.getPassword() == password){
            return true;
        }
    }
    return false;
}