project(forecaster)
cmake_minimum_required(VERSION 2.8)


IF( ${CMAKE_CXX_COMPILER} MATCHES  "[cg][+][+]")
    MESSAGE("G++ ")
    set(CMAKE_CXX_FLAGS "-Wall -std=c++11")
    set(CMAKE_CXX_FLAGS_DEBUG "-g3")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
    set(CMAKE_C_FLAGS "-Wall ")
    set(CMAKE_C_FLAGS_DEBUG "-g3")
    add_definitions(-D_BSD_SOURCE)
    add_definitions(-D__USE_POSIX199309)
    set(CMAKE_C_FLAGS_RELEASE "-O3 -std=gnu99")
    set(CMAKE_CXX_FLAGS "-march=native -mtune=native")
    set(CMAKE_C_FLAGS "-march=native -mtune=native")
ELSE()

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
ADD_LIBRARY(forecaster STATIC ${forecaster_SRCS})
LIST(APPEND py_src signal_calc.py base_config.py  clibapp.cpp)

add_custom_target(py_files DEPENDS forecaster SOURCES ${py_src})

