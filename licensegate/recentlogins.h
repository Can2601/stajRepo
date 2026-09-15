#ifndef RECENTLOGINS_H
#define RECENTLOGINS_H

#include <QObject>
#include <QStringList>

// ── RecentLogins ──────────────────────────────────────────────────────────────
// Persists a list of recently used User IDs in a JSON file so they survive
// app restarts.
//
// Save location (set by Qt via QStandardPaths + org/app name in main.cpp):
//   Windows: C:/Users/<user>/AppData/Local/licensegate/licensegate/recent_logins.json
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
    explicit RecentLogins(QObject *parent = nullptr);

    // Returns saved IDs, newest first.
    Q_INVOKABLE QStringList all() const;

    // Saves an ID. Moves it to the front if it already exists. Caps at 10 entries.
    Q_INVOKABLE void add(const QString &id);

    // Removes one entry by exact ID match.
    Q_INVOKABLE void remove(const QString &id);

private:
    static QString filePath();
    static void save(const QStringList &list);
};

#endif // RECENTLOGINS_H
