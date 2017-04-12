QT -= core 
QT -= gui

CONFIG += release console

TEMPLATE = app
SOURCES += autotools/opencl/main.c  \
    autotools/opencl/config.c  \
    autotools/opencl/prng.c



HEADERS += autotools/opencl/config.h \
    autotools/opencl/prng.h \
    autotools/clibapp.h \
    autotools/clibdef.h


INCLUDEPATH += /usr/local/include ../fmtrader/eal ../fmtrader/lib/libforecaster

OTHER_FILES += CMakeLists.txt \
    autotools/opencl/*.cl   \
    conf_linux.json \
    testlib.c   \
    autotools/clibapp.cpp
