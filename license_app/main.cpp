#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "licensemanager.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    LicenseManager licenseManager;

    QQmlApplicationEngine engine;
    // "licenseManager" adıyla QML tarafına tanıtıyoruz.
    // QML içinde artık licenseManager.generateAndSave(...) diye çağırabiliriz.
    engine.rootContext()->setContextProperty("licenseManager", &licenseManager);

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                      &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
