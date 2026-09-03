cmake_minimum_required(VERSION 3.16)

project(securegate VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)

set(SODIUM_ROOT "C:/Libsodium/libsodium-win64")

set(SODIUM_INCLUDE_DIR "${SODIUM_ROOT}/include")
set(SODIUM_LIBRARY "${SODIUM_ROOT}/lib/libsodium.dll.a")

qt_standard_project_setup(REQUIRES 6.10)

# Content of downloaded design from Figma to Qt or similar
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/importedcontent/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/importedcontent)
endif()

qt_add_executable(appsecuregate
    main.cpp
    FileReader.h FileReader.cpp
    User.h User.cpp
    loginmanager.h loginmanager.cpp
)

qt_add_qml_module(appsecuregate
    URI securegate
    VERSION 1.0
    QML_FILES
        Main.qml
        DashboardPage.qml
    RESOURCES

    SOURCES
        QML_FILES LoginPage.qml
        RESOURCES
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appsecuregate PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appsecuregate
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(appsecuregate
    PRIVATE "${SODIUM_INCLUDE_DIR}"
)

target_link_libraries(appsecuregate
    PRIVATE
        Qt6::Quick
        "${SODIUM_LIBRARY}"
)

install(TARGETS appsecuregate
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)