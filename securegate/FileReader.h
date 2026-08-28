#ifndef FILEREADER_H
#define FILEREADER_H
#include <User.h>
#include <QList>

class FileReader
{
public:
    FileReader();

    QList<User> readUsers();
};

#endif // FILEREADER_H
