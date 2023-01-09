cmake_minimum_required(VERSION 3.5)

project(ANPR_Ground_Control_Station_V2_1 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS network)

#find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core5Compat)


set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        tcpserverclass.cpp
        tcpserverclass.h
        Utilities.h
        Utilities.cpp
        resource.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ANPR_Ground_Control_Station_V2_1
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET ANPR_Ground_Control_Station_V2_1 APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(ANPR_Ground_Control_Station_V2_1 SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(ANPR_Ground_Control_Station_V2_1
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(ANPR_Ground_Control_Station_V2_1 PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
#target_link_libraries(ANPR_Ground_Control_Station_V2_1 PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(ANPR_Ground_Control_Station_V2_1 PRIVATE Qt${QT_VERSION_MAJOR}::Core5Compat)
set_target_properties(ANPR_Ground_Control_Station_V2_1 PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS ANPR_Ground_Control_Station_V2_1
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ANPR_Ground_Control_Station_V2_1)
endif()

#OpenCV
include_directories(C:/opencv/install/include)
target_link_libraries(ANPR_Ground_Control_Station_V2_1  PRIVATE C:/opencv/install/x64/vc16/lib/opencv_world453.lib)

##Darknet
include_directories(C:/darknet-master/src)
include_directories(C:/darknet-master/include)
target_link_libraries(ANPR_Ground_Control_Station_V2_1  PRIVATE C:/darknet-master/darknet.lib)

#include_directories(C:/Darknet/src)
#include_directories(C:/Darknet/include)
#target_link_libraries(ANPR_Ground_Control_Station_V2_1  PRIVATE C:/Darknet/darknet.lib)


##tessaract
include_directories(C:/vcpkg/installed/x64-windows/include)
include_directories(C:/vcpkg/installed/x64-windows/include/tesseract)
include_directories(C:/vcpkg/installed/x64-windows/include/leptonica)
target_link_libraries(ANPR_Ground_Control_Station_V2_1  PRIVATE C:/vcpkg/installed/x64-windows/lib/tesseract41.lib)


install(TARGETS ANPR_Ground_Control_Station_V2_1 DESTINATION ${CMAKE_SOURCE_DIR}/bin)
#required Qt libraries
find_program(WINDEPLOYQT_EXECUTABLE windeployqt)
add_custom_command(TARGET ANPR_Ground_Control_Station_V2_1 POST_BUILD COMMAND "${WINDEPLOYQT_EXECUTABLE}" --dir "${CMAKE_SOURCE_DIR}/bin"  "$<TARGET_FILE:ANPR_Ground_Control_Station_V2_1>" )



