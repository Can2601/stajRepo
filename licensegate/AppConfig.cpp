#include "AppConfig.h"
#include <QSettings>

AppConfig& AppConfig::instance() {
    static AppConfig single;
    return single;
}

void AppConfig::save() {
    QSettings s("licensegate", "licensegate");
    s.setValue("licenseDir", licenseDir);
}

AppConfig::AppConfig() {
    QSettings s("licensegate", "licensegate");
    licenseDir = s.value("licenseDir", QString()).toString();
}
