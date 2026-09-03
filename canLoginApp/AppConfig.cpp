#include "AppConfig.h"
#include <QSettings>

AppConfig& AppConfig::instance() {
    static AppConfig single;
    return single;
}

void AppConfig::save() {
    QSettings s("testQtProject", "LoginApp");
    s.setValue("licenseDir", licenseDir);
}

AppConfig::AppConfig() {
    QSettings s("testQtProject", "LoginApp");
    licenseDir = s.value("licenseDir", QString()).toString();
}
