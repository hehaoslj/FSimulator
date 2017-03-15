project(forecaster)
cmake_minimum_required(VERSION 2.8)

set(MACOSX_RPATH "@executable_path")

add_library(plugin_libs SHARED IMPORTED)

list(APPEND clean_extra_files "")

set(CMAKE_VERBOSE_MAKEFILE off)

IF( ${CMAKE_CXX_COMPILER} MATCHES  "[cg][+][+]")
    MESSAGE("G++ ")
    set(CMAKE_CXX_FLAGS_DEBUG "-g3")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
    set(CMAKE_C_FLAGS_DEBUG "-g3")
    set(CMAKE_C_FLAGS_RELEASE "-O3")
    set(CMAKE_CXX_FLAGS "-Wall  -std=c++11")
    set(CMAKE_C_FLAGS   "-Wall  -std=c99")
    set_property(TARGET plugin_libs PROPERTY IMPORTED_LOCATION
        "${CMAKE_BINARY_DIR}/libguavatask-1.0.so")
    list(APPEND clean_extra_files
        "${CMAKE_BINARY_DIR}/libguavatask-1.0.so")
ELSE()
    MESSAGE("Unspecified compiler")
ENDIF()

if( ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    MESSAGE("Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -mtune=native")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native -mtune=native")
    set_property(TARGET plugin_libs PROPERTY IMPORTED_LOCATION
        "${CMAKE_BINARY_DIR}/libguavatask-1.0.dylib")
    list(APPEND clean_extra_files
        "${CMAKE_BINARY_DIR}/libguavatask-1.0.dylib")
ENDIF()

FIND_LIBRARY(jansson_libs NAMES jansson
                PATHS /usr/local /usr
            )
FIND_PATH(jansson_header jansson.h)

FIND_LIBRARY(libforecaster_static NAMES forecaster
    PATHS ../fmtrader/lib/libforecaster)


#MESSAGE(jansson path is ${jansson_header})

#LIST(REMOVE_ITEM forecaster_SRCS clibapp.cpp)
#Shared EAL Library
#ADD_LIBRARY(eal SHARED ${EAL_SRCS})
#Static EAL Library
#ADD_LIBRARY(forecaster STATIC ${forecaster_SRCS})
LIST(APPEND py_src signal_calc.py base_config.py  clibapp.cpp README.md testlib.py)

add_custom_target(py_files ALL SOURCES ${py_src})


set(CMAKE_INCLUDE_CURRENT_DIR ON)
#set(CMAKE_AUTOMOC ON)

#Qt Property
#INCLUDE_DIRECTORIES(/Users/hehao/work/lib/QtPropertyBrowser/src)

#FIND_PACKAGE(Qt5 REQUIRED core gui network widgets)
#find_package(Qt5Core)
#find_package(Qt5Widgets)
#find_package(Qt5Gui)
#find_package(Qt5Network)


#qt5_wrap_ui(ui_files mainwindow.ui)
#qt5_wrap_cpp(moc_files mainwindow.cpp)
#qt5_add_resources(qrc_files drawable.qrc)
#LIST(APPEND src_files monitor.cpp mainwindow.cpp embedpython.cpp)
#add_executable(monitor ${ui_files}  ${src_files})
#qt5_use_modules(monitor Core Widgets Network Gui)
#target_link_libraries(monitor Qt5::Widgets Qt5::Core Qt5::Network Qt5::Gui PythonLibs)

add_custom_target(GenCFFI ALL)
add_custom_command(
    TARGET GenCFFI
    PRE_BUILD
    COMMAND pypy -O -OO \"${CMAKE_SOURCE_DIR}/autotools/guavatask.py\"
    COMMENT "Compiling python-- ${CMAKE_SOURCE_DIR}/testlib.py"
)



set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${clean_extra_files}")
message("${clean_extra_files}")

#FIND_LIBRARY(plugin_libs NAMES plugin-1.5
#                PATHS ${CMAKE_BINARY_DIR}
#)
#include(LibFindMacros)
#set(plugin_libs_PROCESS_INCLUDES .)
#set(plugin_libs_PROCESS_LIBS ${CMAKE_BINARY_DIR}/libplugin-1.5.dylib)
#libfind_process(plugin_libs)


#set(plugin_libs_INCLUDE_DIRS .)
#set(plugin_libs_LIBRARY_DIR ${CMAKE_BINARY_DIR})
#set(plugin_libs_LIBRARIES ${CMAKE_BINARY_DIR})
#set(plugin_libs-FOUND TRUE)
#set(plugin_libs plugin-1.5)

#message("plugin lib path " ${plugin_libs})
#message("dist " ${CMAKE_BINARY_DIR})
add_executable(testlib
    testlib.c)
add_dependencies(testlib GenCFFI)
#add_dependencies(testlib forecastertask)
target_link_libraries(testlib plugin_libs)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

INCLUDE_DIRECTORIES(../fmtrader/lib/libforecaster ${jansson_header} )
LIST(APPEND forecaster_SRCS  forecaster.cpp)
LIST(APPEND forecaster_SRCS autotools/clibapp.cpp)

ADD_LIBRARY(forecaster SHARED ${forecaster_SRCS})
target_link_libraries(forecaster ${jansson_libs} ${libforecaster_static})

target_link_libraries(testlib -L${CMAKE_BINARY_DIR}  forecastertask-1.0)
target_link_libraries(testlib -L${CMAKE_BINARY_DIR}  optimizetask-1.0)

add_custom_command(
    TARGET testlib
    PRE_BUILD
    DEPENDS forecaster
    COMMAND pypy -O -OO \"${CMAKE_SOURCE_DIR}/autotools/forecastertask.py\"
    COMMENT "Compiling Forecaster python-- ${CMAKE_SOURCE_DIR}/testlib.py"
)

add_custom_command(
    TARGET testlib
    PRE_BUILD
    DEPENDS forecaster
    COMMAND python -O -OO \"${CMAKE_SOURCE_DIR}/autotools/optimizetask.py\"
    COMMENT "Compiling Forecaster python-- ${CMAKE_SOURCE_DIR}/testlib.py"
)



ELSE(MATCHES "Darwin")
message("Forecaster Dose NOT support OSX")

ELSE(MATCHES "Windows")

ENDIF()

add_custom_command(
    TARGET GenCFFI
    POST_BUILD
    COMMAND cp -r \"${CMAKE_SOURCE_DIR}/autotools\" ${CMAKE_BINARY_DIR}
    COMMENT "Copying autotools"
)

