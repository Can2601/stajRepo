#include "AppSettings.h"
#include "AppConfig.h"

AppSettings::AppSettings(QObject *parent) : QObject(parent) {}

QString AppSettings::licenseDir() const
{
    return AppConfig::instance().licenseDir;
}

void AppSettings::setLicenseDirFromUrl(const QUrl &url)
{
    const QString path = url.toLocalFile();
    if (path.isEmpty() || path == AppConfig::instance().licenseDir)
        return;

    AppConfig::instance().licenseDir = path;
    AppConfig::instance().save();
    emit licenseDirChanged();
}
