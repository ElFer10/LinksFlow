cmake_minimum_required(VERSION 3.19)
project(LinksFlow LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

qt_add_executable(LinksFlow
    WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    mainwindow.ui
    src/ui/configurationpage.h
    src/ui/configurationpage.cpp
    src/ui/configuration/resolutiontab.h
    src/ui/configuration/resolutiontab.cpp
)

target_link_libraries(LinksFlow
    PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Widgets
)

install(TARGETS LinksFlow
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET LinksFlow
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
