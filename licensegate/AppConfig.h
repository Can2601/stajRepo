#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QString>

class AppConfig {
public:
    static AppConfig& instance();

    QString licenseDir;

    // Persist current values to QSettings.
    void save();

private:
    // Load saved values on first construction.
    AppConfig();

    AppConfig(const AppConfig&)            = delete;
    AppConfig& operator=(const AppConfig&) = delete;
};

#endif // APPCONFIG_H