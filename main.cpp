#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "licenseauth.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Expose LicenseAuth to QML as a context property.
    // QML calls: LicenseAuth.validateCredentials(id, pwd)
    //            LicenseAuth.lastError()
    LicenseAuth licenseAuth;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("LicenseAuth", &licenseAuth);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("testQtProject", "Main");

    return QGuiApplication::exec();
}
