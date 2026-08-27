#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "licenseauth.h"
#include "recentlogins.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("testQtProject");
    app.setApplicationName("LoginApp");

    // Expose LicenseAuth to QML as a context property.
    // QML calls: LicenseAuth.validateCredentials(id, pwd)
    //            LicenseAuth.lastError()
    LicenseAuth licenseAuth;

    // Expose RecentLogins to QML as a context property.
    // QML calls: RecentLogins.all() / RecentLogins.add(id) / RecentLogins.remove(id)
    RecentLogins recentLogins;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("LicenseAuth",   &licenseAuth);
    engine.rootContext()->setContextProperty("RecentLogins",  &recentLogins);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("testQtProject", "Main");

    return QGuiApplication::exec();
}
