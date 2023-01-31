cmake_minimum_required(VERSION 3.5)

project(mockDbGeneratorUI VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(POSSIBLE_RELEASE_STRINGS "")
list(
    APPEND POSSIBLE_RELEASE_STRINGS
    "Release"
    "RelWithDebInfo"
    "MinSizeRel"
)

set(RELEASE_BUILD false)
foreach(POSSIBLE_STRING ${POSSIBLE_RELEASE_STRINGS})
    if (CMAKE_BUILD_TYPE STREQUAL ${POSSIBLE_STRING} OR CMAKE_CONFIGURATION_TYPES STREQUAL ${POSSIBLE_STRING})
        message(STATUS "Matched with ${POSSIBLE_STRING}")
        set(RELEASE_BUILD true)
        break()
    endif()
endforeach()

if (NOT DEFINED ENV{GITHUB_ACTION})
    set(STATIC_QT_DIR "C:/Qt6Static")
else()
    message(STATUS "In github actions, static qt dir = ${STATIC_QT_DIR}")
endif()

if (NOT STATIC_QT_DIR AND RELEASE_BUILD)
    message(FATAL_ERROR "RELEASE_BUILD was specified but STATIC_QT_DIR was not passed")
elseif(RELEASE_BUILD)
    set(STATIC_BUILD true)
    set(CMAKE_PREFIX_PATH ${STATIC_QT_DIR})
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        mocktable.h
        mocktable.cpp
        mockattribute.h
        mockattribute.cpp
        resources.qrc
)
set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/resources/mockDbGeneratorUI.rc")
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(mockDbGeneratorUI
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${app_icon_resource_windows}
    )
else()
    message(FATAL_ERROR "Needs QT 6 at least")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(mockDbGeneratorUI PRIVATE QT_NO_DEBUG_OUTPUT)
endif()

target_link_libraries(mockDbGeneratorUI PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(mockDbGeneratorUI PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS mockDbGeneratorUI
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(mockDbGeneratorUI)
endif()
