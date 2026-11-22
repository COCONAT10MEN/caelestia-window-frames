cmake_minimum_required(VERSION 3.22)

project(caelestia-window-frames VERSION 1.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QT_MIN_VERSION "6.6.0")
set(KF6_MIN_VERSION "6.0.0")

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Gui)
find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS CoreAddons)
find_package(KDecoration3 REQUIRED)

add_library(caelestiawindowframes MODULE
    src/plugin.cpp
    src/theme.cpp
    src/decoration.cpp
    src/button.cpp
    src/icons.cpp
    src/motion.cpp
    src/statelayer.cpp
)

set_target_properties(caelestiawindowframes PROPERTIES
    OUTPUT_NAME org.caelestia.windowframes
)

target_link_libraries(caelestiawindowframes
    PRIVATE
        Qt6::Core
        Qt6::Gui
        KF6::CoreAddons
        KDecoration3::KDecoration
)

install(TARGETS caelestiawindowframes DESTINATION ${KDE_INSTALL_PLUGINDIR}/org.kde.kdecoration3)
