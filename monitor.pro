QT       += core gui
QT += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += qscintilla2 release

TARGET = monitor

TEMPLATE = app

SOURCES += monitor.cpp  \
    mainwindow.cpp  \
    embedpython.cpp

HEADERS += mainwindow.h \
    embedpython.h   \
    stdafx.h \
    clibapp.h \
    clibdef.h

FORMS += mainwindow.ui

macx-clang {
LIBS += /usr/local/lib/libjansson.a
INCLUDEPATH += /usr/local/include   \
    /Users/hehao/work/lib/libforecaster

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.11

#Python
QMAKE_LFLAGS += -F/opt/local/Library/Frameworks -F/usr/local/opt/python/Frameworks
INCLUDEPATH += /opt/local/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7
INCLUDEPATH += /usr/local/opt/python/Frameworks/Python.framework/Versions/2.7/include/python2.7
LIBS += -framework Python

}

include("../lib/QtPropertyBrowser/src/qtpropertybrowser.pri")

OTHER_FILES += meta.json signal_calc.py \
    autotools/clib*.* \
    autotools/*.h   \
    /Users/hehao/work/lib/libforecaster/*.h \
    conf_linux.json autotools/*.py

RESOURCES += \
    monitor.qrc

DISTFILES += \
    autotools/__init__.py \
    autotools/builder.py \
    autotools/signal_calc.py \
    autotools/trading_signal.py \
    autotools/market_calc.py


