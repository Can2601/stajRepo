cmake_minimum_required(VERSION 3.16)

project(testQtProject VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── libsodium ──────────────────────────────────────────────────────────────
# Adjust this path if you move the library folder.
set(SODIUM_ROOT "C:/Users/cmkak/Documents/libs/libsodium-win64"
    CACHE PATH "Path to the libsodium installation")

find_package(Qt6 REQUIRED COMPONENTS Quick QuickDialogs2)

qt_standard_project_setup(REQUIRES 6.10)

# Content of downloaded design from Figma to Qt or similar
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/importedcontent/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/importedcontent)
endif()

qt_add_executable(apptestQtProject
    main.cpp
    licenseauth.h       # Q_OBJECT – MOC must process this
    recentlogins.h      # Q_OBJECT – MOC must process this
    AppConfig.h
    AppSettings.h       # Q_OBJECT – MOC must process this
    crypto_utils.h
    shared_license_key.h
    public_license_key.h
)

qt_add_qml_module(apptestQtProject
    URI testQtProject
    QML_FILES
        Main.qml
        LoginPage.qml
        WelcomePage.qml
)

set_target_properties(apptestQtProject PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.apptestQtProject
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(apptestQtProject PRIVATE ${SODIUM_ROOT}/include)
target_link_libraries(apptestQtProject
    PRIVATE Qt6::Quick
    PRIVATE Qt6::QuickDialogs2
    PRIVATE ${SODIUM_ROOT}/lib/libsodium.a
)

install(TARGETS apptestQtProject
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
