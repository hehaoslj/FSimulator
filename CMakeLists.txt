project(forecaster)
cmake_minimum_required(VERSION 2.8)


IF( ${CMAKE_CXX_COMPILER} MATCHES  "[cg][+][+]")
    MESSAGE("G++ ")
    set(CMAKE_CXX_FLAGS "-Wall -std=c++11")
    set(CMAKE_CXX_FLAGS_DEBUG "-g3")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
    set(CMAKE_C_FLAGS "-Wall -std=c99")
    set(CMAKE_C_FLAGS_DEBUG "-g3")
    set(CMAKE_C_FLAGS_RELEASE "-O3")
    set(CMAKE_CXX_FLAGS "-march=native -mtune=native")
    set(CMAKE_C_FLAGS "-march=native -mtune=native")
ELSE()
    MESSAGE("Unspecified compiler")
ENDIF()

FIND_LIBRARY(jansson_libs NAMES jansson
                PATHS /usr/local /usr
            )
FIND_PATH(jansson_header jansson.h)



#MESSAGE(jansson path is ${jansson_header})
INCLUDE_DIRECTORIES(../fmtrader/lib/libforecaster ${jansson_header})
LIST(APPEND forecaster_SRCS  forecaster.cpp)
LIST(APPEND forecaster_SRCS clibapp.cpp)
#LIST(REMOVE_ITEM forecaster_SRCS clibapp.cpp)
#Shared EAL Library
#ADD_LIBRARY(eal SHARED ${EAL_SRCS})
#Static EAL Library
#ADD_LIBRARY(forecaster STATIC ${forecaster_SRCS})
LIST(APPEND py_src signal_calc.py base_config.py  clibapp.cpp README.md)

add_custom_target(py_files DEPENDS forecaster SOURCES ${py_src})


set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

#FIND_PACKAGE(Qt5 REQUIRED core gui network widgets)
find_package(Qt5Core)
find_package(Qt5Widgets)
find_package(Qt5Gui)
find_package(Qt5Network)

qt5_wrap_ui(ui_files mainwindow.ui)
qt5_wrap_cpp(moc_files mainwindow.cpp)
qt5_add_resources(qrc_files drawable.qrc)
LIST(APPEND src_files monitor.cpp mainwindow.cpp)

add_executable(monitor ${ui_files}  ${src_files})
#qt5_use_modules(monitor Core Widgets Network Gui)
target_link_libraries(monitor Qt5::Widgets Qt5::Core Qt5::Network Qt5::Gui)

