#include "recentlogins.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>

RecentLogins::RecentLogins(QObject *parent) : QObject(parent) {}

QStringList RecentLogins::all() const
{
    QFile file(filePath());
    if (!file.open(QIODevice::ReadOnly))
        return {};                          // file doesn't exist yet → empty list

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return {};

    QJsonArray arr = doc.object()["ids"].toArray();
    QStringList list;
    for (const QJsonValue &v : arr)
        list << v.toString();
    return list;
}

void RecentLogins::add(const QString &id)
{
    if (id.trimmed().isEmpty()) return;
    QStringList list = all();
    list.removeAll(id);          // remove duplicate if present
    list.prepend(id);            // newest at the top
    if (list.size() > 10)
        list = list.mid(0, 10); // keep at most 10
    save(list);
}

void RecentLogins::remove(const QString &id)
{
    QStringList list = all();
    list.removeAll(id);
    save(list);
}

QString RecentLogins::filePath()
{
    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dir);          // create the directory if it doesn't exist yet
    return dir + "/recent_logins.json";
}

void RecentLogins::save(const QStringList &list)
{
    QJsonArray arr;
    for (const QString &s : list)
        arr << s;

    QJsonObject root;
    root["ids"] = arr;

    QFile file(filePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}
