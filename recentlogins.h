#ifndef RECENTLOGINS_H
#define RECENTLOGINS_H

#include <QObject>
#include <QStringList>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>

// ── RecentLogins ──────────────────────────────────────────────────────────────
// Persists a list of recently used User IDs in a JSON file so they survive
// app restarts.
//
// Save location (set by Qt via QStandardPaths + org/app name in main.cpp):
//   Windows: C:/Users/<user>/AppData/Local/testQtProject/LoginApp/recent_logins.json
//
// File format:
//   {
//       "ids": ["user123", "user456", "user789"]
//   }
//
// QML usage (via "RecentLogins" context property):
//   RecentLogins.all()           → JS string array of saved IDs (newest first)
//   RecentLogins.add("someId")   → save an ID (de-duped, max 10 entries)
//   RecentLogins.remove("id")    → delete one entry
// ─────────────────────────────────────────────────────────────────────────────
class RecentLogins : public QObject
{
    Q_OBJECT
public:
    explicit RecentLogins(QObject *parent = nullptr) : QObject(parent) {}

    // Returns saved IDs, newest first.
    Q_INVOKABLE QStringList all() const
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

    // Saves an ID. Moves it to the front if it already exists. Caps at 10 entries.
    Q_INVOKABLE void add(const QString &id)
    {
        if (id.trimmed().isEmpty()) return;
        QStringList list = all();
        list.removeAll(id);          // remove duplicate if present
        list.prepend(id);            // newest at the top
        if (list.size() > 10)
            list = list.mid(0, 10); // keep at most 10
        save(list);
    }

    // Removes one entry by exact ID match.
    Q_INVOKABLE void remove(const QString &id)
    {
        QStringList list = all();
        list.removeAll(id);
        save(list);
    }

private:
    // Returns the full path to the JSON file.
    // QStandardPaths::AppLocalDataLocation resolves to:
    //   Windows → C:/Users/<user>/AppData/Local/<OrgName>/<AppName>/
    // The org/app names are set via app.setOrganizationName / setApplicationName
    // in main.cpp, so the final path is:
    //   C:/Users/<user>/AppData/Local/testQtProject/LoginApp/recent_logins.json
    static QString filePath()
    {
        const QString dir =
            QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        QDir().mkpath(dir);          // create the directory if it doesn't exist yet
        return dir + "/recent_logins.json";
    }

    // Serialises the list and writes it to disk as indented JSON.
    static void save(const QStringList &list)
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
};

#endif // RECENTLOGINS_H
