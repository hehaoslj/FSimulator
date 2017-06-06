cmake_minimum_required(VERSION 2.8)

project(fsimulator)

set(CMAKE_VERBOSE_MAKEFILE off)

#pypy include path /usr/lib64/pypy-5.0.1/include

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)


IF (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    #shared lib suffix
    set(dll_ext ".so")

    #OpenCL
    find_package(OpenCL)
    if(OPENCL_FOUND)
        INCLUDE_DIRECTORIES(${OpenCL_INCLUDE_DIRS} /usr/local/include)
        LINK_DIRECTORIES(${OpenCL_LIBRARY})
    else()
        #OpenCL cuda7.5 version
        set(OpenCL_DIR /usr/local/cuda-7.5)
        set(OpenCL_LIBRARIES OpenCL)
        INCLUDE_DIRECTORIES(${OpenCL_DIR}/include)
        LINK_DIRECTORIES(${OpenCL_DIR}/lib64)
    endif()

ELSE(MATCHES "Darwin")
    #shared lib suffix
    set(dll_ext ".dylib")

    #Set osx rpath
    set( CMAKE_MACOSX_RPATH 1)
    set( MACOSX_RPATH "@executable_path")

    #OpenCL
    find_package(OpenCL REQUIRED)
    INCLUDE_DIRECTORIES(${OpenCL_INCLUDE_DIRS} /usr/local/include)
    LINK_DIRECTORIES(${OpenCL_LIBRARY})

ELSE(MATCHES "Windows")
    #shared lib suffix
    set(dll_ext ".dll")
ENDIF()


# Use the compile definitions defined in the Qt 5 Widgets module
#add_definitions


list(APPEND clean_extra_files "")


IF( ${CMAKE_CXX_COMPILER} MATCHES  "[cg][+][+]")
    MESSAGE("G++ ")
    set(CMAKE_CXX_FLAGS_DEBUG "-g3")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
    set(CMAKE_C_FLAGS_DEBUG "-g3")
    set(CMAKE_C_FLAGS_RELEASE "-O3")
    set(CMAKE_CXX_FLAGS "-Wall  -std=c++11 -march=native -mtune=native")
    set(CMAKE_C_FLAGS   "-Wall  -std=gnu99 -march=native -mtune=native")

ELSE()
    MESSAGE("Unspecified compiler")
ENDIF()

if( ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    MESSAGE("Clang can try -DCMAKE_C_COMPILER=gcc-6 -DCMAKE_CXX_COMPILER=g++-6")
ENDIF()

#OpenMP
FIND_PACKAGE( OpenMP)
if(OPENMP_FOUND)
message("OPENMP FOUND")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

FIND_LIBRARY(jansson_libs NAMES jansson
                PATHS /usr/local /usr
            )
FIND_PATH(jansson_header jansson.h)
INCLUDE_DIRECTORIES(${jansson_header})


##LIB EAL
FIND_PATH(eal_header lmice_trace.h
    PATHS ../fmtrader/eal)
LIST(APPEND eal_SRCS  ../fmtrader/eal/lmice_ring.c
    ../fmtrader/eal/lmice_trace.c
    ../fmtrader/eal/lmice_eal_shm.c
    ../fmtrader/eal/lmice_eal_hash.c
    ../fmtrader/eal/lmice_eal_spinlock.c
    ../fmtrader/eal/lmice_eal_malloc.c
    ../fmtrader/eal/lmice_eal_event.c
    ../fmtrader/eal/lmice_eal_time.c
    ../fmtrader/eal/lmice_core.c
    ../fmtrader/eal/lmice_eal_aio.c
    ../fmtrader/eal/lmice_eal_thread.c
    ../fmtrader/eal/lmice_bloomfilter.c
    )

#Shared EAL Library
#ADD_LIBRARY(eal SHARED ${EAL_SRCS})
#Static EAL Library
ADD_LIBRARY(eal_static STATIC ${eal_SRCS})

SET_TARGET_PROPERTIES(eal_static PROPERTIES OUTPUT_NAME "eal")
SET_TARGET_PROPERTIES(eal_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
#SET_TARGET_PROPERTIES(eal_static PROPERTIES VERSION 1.2 SOVERSION 1)

INCLUDE_DIRECTORIES(${eal_header})

#Static forecaster lib
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    INCLUDE_DIRECTORIES(../fmtrader/lib/libforecaster)
    add_library(libforecaster_static STATIC IMPORTED)
    set_property(TARGET libforecaster_static PROPERTY IMPORTED_LOCATION
        "${CMAKE_SOURCE_DIR}/../fmtrader/lib/libforecaster/libforecaster.a")


#    FIND_LIBRARY(libforecaster_static NAMES forecaster
#        PATHS ../fmtrader/lib/libforecaster)
    message("libforecaster ${libforecaster_static}")

ELSE(MATCHES "Darwin")
    INCLUDE_DIRECTORIES(../lib/forecaster)
    LIST(APPEND libforecaster_SRCS
        ../lib/forecaster/ChinaL1DiscreteFeature.cpp
        ../lib/forecaster/Feature.cpp
        ../lib/forecaster/forecaster.cpp
        ../lib/forecaster/RollScheme.cpp
        )
    ADD_LIBRARY(libforecaster_static STATIC ${libforecaster_SRCS})

    SET_TARGET_PROPERTIES(libforecaster_static PROPERTIES OUTPUT_NAME "forecaster")
    SET_TARGET_PROPERTIES(libforecaster_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
    #SET_TARGET_PROPERTIES(libforecaster_static PROPERTIES VERSION 1.2 SOVERSION 1)
    message("libforecaster ${libforecaster_static}")
ENDIF()

#Shared forecaster lib
LIST(APPEND forecaster_SRCS
    forecaster.cpp
    autotools/clibapp.cpp
    )
ADD_LIBRARY(forecaster_shared SHARED ${forecaster_SRCS})
SET_TARGET_PROPERTIES(forecaster_shared PROPERTIES OUTPUT_NAME "forecaster")
target_link_libraries(forecaster_shared
    ${jansson_libs}
    eal_static
    libforecaster_static

)
ADD_DEPENDENCIES(forecaster_shared eal_static)

#Extra files
LIST(APPEND py_src conf_linux.json meta.json)
AUX_SOURCE_DIRECTORY(autotools/opencl py_src)
AUX_SOURCE_DIRECTORY(autotools py_src)
LIST(APPEND py_src autotools/base_config.py
    autotools/forecasterhandle.py
    autotools/forecastertask.py
    autotools/guavahandle.py
    autotools/guavatask.py
    autotools/optimizehandle.py
    autotools/optimizetask.py
    autotools/testpy.py
    autotools/clibdef.h

    autotools/opencl/signalcalc_tmpl.cl
    autotools/opencl/signalcalchandle.py
    autotools/opencl/signalcalctask.py
    )
add_custom_target(py_files ALL SOURCES ${py_src})




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

add_custom_target(GenOpenCL ALL)

add_custom_target(GenCFFI ALL)

#Shared guavatask Pypy
add_custom_command(
    TARGET GenCFFI
    PRE_BUILD
    COMMAND pypy -O -OO \"${CMAKE_SOURCE_DIR}/autotools/guavatask.py\"
    COMMENT "Compiling python-- ${CMAKE_SOURCE_DIR}/autotools/guavatask.py"
)
add_library(guavatask_lib SHARED IMPORTED)
set_property(TARGET guavatask_lib PROPERTY IMPORTED_LOCATION
    "${CMAKE_BINARY_DIR}/libguavatask-1.0${dll_ext}")
list(APPEND clean_extra_files "${CMAKE_BINARY_DIR}/guavatask.c"
    "${CMAKE_BINARY_DIR}/guavatask.o"
    "${CMAKE_BINARY_DIR}/libguavatask-1.0${dll_ext}")

#Shared signalcalctask Pypy
add_custom_command(
    TARGET GenCFFI
    PRE_BUILD
    COMMAND pypy -O -OO \"${CMAKE_SOURCE_DIR}/autotools/opencl/signalcalctask.py\"
    COMMENT "Compiling signalcalc-Task python-- ${CMAKE_SOURCE_DIR}/autotools/signalcalctask.py"
)
add_library(signalcalctask_lib SHARED IMPORTED)
set_property(TARGET signalcalctask_lib PROPERTY IMPORTED_LOCATION
    "${CMAKE_BINARY_DIR}/libsignalcalctask-1.0${dll_ext}")
list(APPEND clean_extra_files "${CMAKE_BINARY_DIR}/signalcalctask.c"
    "${CMAKE_BINARY_DIR}/signalcalctask.o"
    "${CMAKE_BINARY_DIR}/libsignalcalctask-1.0${dll_ext}")

#Shared forecastertask Pypy
add_custom_command(
    TARGET forecaster_shared
    POST_BUILD
    COMMAND pypy -O -OO \"${CMAKE_SOURCE_DIR}/autotools/forecastertask.py\"
    COMMENT "Compiling Forecaster-Task python-- ${CMAKE_SOURCE_DIR}/autotools/forecastertask.py"
)
add_library(forecastertask_lib SHARED IMPORTED)
set_property(TARGET forecastertask_lib PROPERTY IMPORTED_LOCATION
    "${CMAKE_BINARY_DIR}/libforecastertask-1.0${dll_ext}")
list(APPEND clean_extra_files "${CMAKE_BINARY_DIR}/forecastertask.c"
    "${CMAKE_BINARY_DIR}/forecastertask.o"
    "${CMAKE_BINARY_DIR}/libforecastertask-1.0${dll_ext}")


#Shared optimizetask Pypy
add_custom_command(
    TARGET GenCFFI
    PRE_BUILD
    COMMAND pypy -O -OO \"${CMAKE_SOURCE_DIR}/autotools/optimizetask.py\"
    COMMENT "Compiling Optimizer-Task python-- ${CMAKE_SOURCE_DIR}/autotools/optimizetask.py"
)
add_library(optimizetask_lib SHARED IMPORTED)
set_property(TARGET optimizetask_lib PROPERTY IMPORTED_LOCATION
    "${CMAKE_BINARY_DIR}/liboptimizetask-1.0${dll_ext}")
list(APPEND clean_extra_files "${CMAKE_BINARY_DIR}/optimizetask.c"
    "${CMAKE_BINARY_DIR}/optimizetask.o"
    "${CMAKE_BINARY_DIR}/liboptimizetask-1.0${dll_ext}")

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${clean_extra_files}")


#App testlib cpp
add_executable(testlib
    testlib.c)
add_dependencies(testlib GenCFFI)
add_dependencies(testlib forecaster_shared)
target_link_libraries(testlib
    ${guavatask_lib}
    ${optimizetask_lib}
    forecastertask_lib
    ${signalcalctask_lib}
)

#App myopencl opencl
LIST(APPEND myopencl_src
    autotools/opencl/main.c
    autotools/opencl/prng.c
    autotools/opencl/config.c
    autotools/opencl/verify.c
    )
add_executable(myopencl ${myopencl_src})
add_dependencies(myopencl GenCFFI)
add_dependencies(myopencl eal_static)
target_link_libraries(myopencl
    ${jansson_libs}
    eal_static
    signalcalctask_lib
    ${OpenCL_LIBRARIES}
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    target_link_libraries(myopencl /lib64/libpthread.so.0)
    target_link_libraries(myopencl m)


ELSE(MATCHES "Darwin")


ELSE(MATCHES "Windows")

ENDIF()

add_custom_command(
    TARGET GenCFFI
    POST_BUILD
    COMMAND cp -r \"${CMAKE_SOURCE_DIR}/autotools\" ${CMAKE_BINARY_DIR}
    COMMENT "Copying autotools[python]"
)

add_custom_command(
    TARGET GenCFFI
    POST_BUILD
    COMMAND cp -r \"${CMAKE_SOURCE_DIR}/web\" ${CMAKE_BINARY_DIR}
    COMMENT "Copying web[python]"
)
