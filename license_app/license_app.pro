QT += quick quickcontrols2

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp

HEADERS += \
        licensemanager.h \
        shared_license_key.h

RESOURCES += qml.qrc

# ── libsodium ─────────────────────────────────────────────────────────────────
# Link statically (libsodium.a) so no libsodium-26.dll is needed at runtime.
SODIUM_ROOT = C:\Libsodium\libsodium-win64

INCLUDEPATH += $$SODIUM_ROOT/include
LIBS        += $$SODIUM_ROOT/lib/libsodium.a
win32: LIBS += -lpthread
# ─────────────────────────────────────────────────────────────────────────────

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
