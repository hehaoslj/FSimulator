cmake_minimum_required(VERSION 2.8)

project(fsimulator)

set(CMAKE_VERBOSE_MAKEFILE off)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Set osx rpath
set(MACOSX_RPATH "@executable_path")

# Use the compile definitions defined in the Qt 5 Widgets module
#add_definitions

add_library(plugin_libs SHARED IMPORTED)

list(APPEND clean_extra_files "")



IF( ${CMAKE_CXX_COMPILER} MATCHES  "[cg][+][+]")
    MESSAGE("G++ ")
    set(CMAKE_CXX_FLAGS_DEBUG "-g3")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
    set(CMAKE_C_FLAGS_DEBUG "-g3")
    set(CMAKE_C_FLAGS_RELEASE "-O3")
    set(CMAKE_CXX_FLAGS "-Wall  -std=c++11")
    set(CMAKE_C_FLAGS   "-Wall  -std=gnu99")
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

#MESSAGE(jansson path is ${eal_header})
INCLUDE_DIRECTORIES(${jansson_header})
INCLUDE_DIRECTORIES(${eal_header})

#LIST(REMOVE_ITEM forecaster_SRCS clibapp.cpp)
#Shared EAL Library
#ADD_LIBRARY(eal SHARED ${EAL_SRCS})
#Static EAL Library
#ADD_LIBRARY(forecaster STATIC ${forecaster_SRCS})
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
target_link_libraries(testlib -L${CMAKE_BINARY_DIR}  signalcalctask-1.0)


add_custom_command(
    TARGET testlib
    PRE_BUILD
    DEPENDS GenCFFI
    COMMAND pypy -O -OO \"${CMAKE_SOURCE_DIR}/autotools/opencl/signalcalctask.py\"
    COMMENT "Compiling signalcalc-opencl python-- ${CMAKE_SOURCE_DIR}/autotools/opencl/signalcalctask.py"
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    #pypy include path /usr/lib64/pypy-5.0.1/include
    set(OpenCL_DIR /usr/local/cuda-7.5)
    INCLUDE_DIRECTORIES(${OpenCL_DIR}/include)
    LINK_DIRECTORIES(${OpenCL_DIR}/lib64)
    target_link_libraries(testlib -L${CMAKE_BINARY_DIR} forecastertask-1.0)

    LIST(APPEND myopencl_src autotools/opencl/main.c autotools/opencl/prng.c  autotools/opencl/config.c)
    add_executable(myopencl ${myopencl_src})
    target_link_libraries(myopencl ${jansson_libs} eal_static OpenCL /lib64/libpthread.so.0)
    target_link_libraries(myopencl -L${CMAKE_BINARY_DIR}  signalcalctask-1.0)

    add_custom_command(
            TARGET myopencl
            POST_BUILD
            COMMAND cp -r \"${CMAKE_SOURCE_DIR}/autotools/opencl/subsignal_trade.cl\" ${CMAKE_BINARY_DIR}
            COMMENT "Copying opencl script"
        )

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
        COMMAND pypy -O -OO \"${CMAKE_SOURCE_DIR}/autotools/optimizetask.py\"
        COMMENT "Compiling Forecaster python-- ${CMAKE_SOURCE_DIR}/testlib.py"
    )



ELSE(MATCHES "Darwin")
message("Forecaster Dose NOT support OSX")
#OpenCL

    find_package(OpenCL REQUIRED)

    INCLUDE_DIRECTORIES(${OpenCL_INCLUDE_DIRS} /usr/local/include)

    LINK_DIRECTORIES(${OpenCL_LIBRARY})

    message("${OpenCL_LIBRARIES}")
    add_custom_command(
        TARGET GenOpenCL

        COMMAND /System/Library/Frameworks/OpenCL.framework/Libraries/openclc -c -cl-std=CL1.2 -cl-auto-vectorize-enable -O3 \"${CMAKE_SOURCE_DIR}/autotools/opencl/mykernel.cl\"
        COMMENT "Compiling opencl"
    )

#FIND_PACKAGE( OpenMP REQUIRED)
if(OPENMP_FOUND)
message("OPENMP FOUND")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

    LIST(APPEND myopencl_src autotools/opencl/main.c autotools/opencl/prng.c  autotools/opencl/config.c)
    add_executable(myopencl ${myopencl_src})
    target_link_libraries(myopencl ${OpenCL_LIBRARIES} ${jansson_libs} eal_static)
    target_link_libraries(myopencl -L${CMAKE_BINARY_DIR}  signalcalctask-1.0)

    add_custom_command(
        TARGET myopencl
        POST_BUILD
        #COMMAND cp -r \"${CMAKE_SOURCE_DIR}/autotools/opencl/subsignal_trade.cl\" ${CMAKE_BINARY_DIR}
        COMMENT "Copying opencl script"
    )

ELSE(MATCHES "Windows")

ENDIF()

add_custom_command(
    TARGET GenCFFI
    POST_BUILD
    COMMAND cp -r \"${CMAKE_SOURCE_DIR}/autotools\" ${CMAKE_BINARY_DIR}
    COMMENT "Copying autotools"
)
