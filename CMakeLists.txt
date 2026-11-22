cmake_minimum_required(VERSION 3.22)

project(caelestia-window-frames VERSION 1.3.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QT_MIN_VERSION "6.6.0")
set(KF6_MIN_VERSION "6.0.0")

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_FIND_MODULE_DIR})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Gui Widgets Quick DBus)
find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS CoreAddons)
find_package(KDecoration3 REQUIRED)
# KWin itself: the decoration reads its window's place on screen from KWin,
# which the decoration API does not pass on. Rebuild after KWin updates.
find_package(KWin REQUIRED)

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
        KWin::kwin
)

install(TARGETS caelestiawindowframes DESTINATION ${KDE_INSTALL_PLUGINDIR}/org.kde.kdecoration3)
