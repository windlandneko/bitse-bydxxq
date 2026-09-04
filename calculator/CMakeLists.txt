cmake_minimum_required(VERSION 3.16)

project(calculator VERSION 1.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        calculatormodel.cpp
        calculatormodel.h
        draggablebutton.cpp
        draggablebutton.h
        financialdialog.cpp
        financialdialog.h
        financialdialog.ui
        resources.qrc
)

qt_add_executable(calculator
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

target_link_libraries(calculator PRIVATE Qt6::Widgets)

set_target_properties(calculator PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

qt_finalize_executable(calculator)
