#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "licenseauth.h"
#include "recentlogins.h"
#include "AppConfig.h"
#include "AppSettings.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("testQtProject");
    app.setApplicationName("LoginApp");

    // Expose LicenseAuth to QML as a context property.
    // QML calls: LicenseAuth.validateCredentials(id, pwd)
    //            LicenseAuth.lastError()
    LicenseAuth  licenseAuth;
    RecentLogins recentLogins;
    AppSettings  appSettings;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("LicenseAuth",  &licenseAuth);
    engine.rootContext()->setContextProperty("RecentLogins", &recentLogins);
    engine.rootContext()->setContextProperty("AppSettings",  &appSettings);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("testQtProject", "Main");

    return QGuiApplication::exec();
}
