#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QObject>
#include <QUrl>
#include "AppConfig.h"

// ── AppSettings ───────────────────────────────────────────────────────────────
// QObject wrapper around AppConfig, exposed to QML as "AppSettings".
//
// Provides the licenseDir property with change notification so QML bindings
// update automatically when the folder is changed.
//
// QML usage:
//   AppSettings.licenseDir                          → current path (string)
//   AppSettings.setLicenseDirFromUrl(selectedFolder) → set from FolderDialog URL
// ─────────────────────────────────────────────────────────────────────────────
class AppSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString licenseDir READ licenseDir NOTIFY licenseDirChanged)

public:
    explicit AppSettings(QObject *parent = nullptr) : QObject(parent) {}

    // Returns the currently configured license directory path.
    QString licenseDir() const
    {
        return AppConfig::instance().licenseDir;
    }

    // Accepts the QUrl that FolderDialog.selectedFolder returns,
    // converts it to a local filesystem path, persists it, and notifies QML.
    Q_INVOKABLE void setLicenseDirFromUrl(const QUrl &url)
    {
        const QString path = url.toLocalFile();
        if (path.isEmpty() || path == AppConfig::instance().licenseDir)
            return;

        AppConfig::instance().licenseDir = path;
        AppConfig::instance().save();
        emit licenseDirChanged();
    }

signals:
    void licenseDirChanged();
};

#endif // APPSETTINGS_H
