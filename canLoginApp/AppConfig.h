#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QString>
#include <QSettings>

class AppConfig {
public:
    static AppConfig& instance() {
        static AppConfig single;
        return single;
    }

    QString licenseDir;

    // Persist current values to QSettings.
    void save() {
        QSettings s("testQtProject", "LoginApp");
        s.setValue("licenseDir", licenseDir);
    }

private:
    // Load saved values on first construction.
    AppConfig() {
        QSettings s("testQtProject", "LoginApp");
        licenseDir = s.value("licenseDir", QString()).toString();
    }

    AppConfig(const AppConfig&)            = delete;
    AppConfig& operator=(const AppConfig&) = delete;
};

#endif // APPCONFIG_H