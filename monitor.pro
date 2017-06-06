QT       += core gui
QT       += network

QT       += webkit webkitwidgets
#QT += webenginewidgets

QT       += websockets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += release

TARGET = monitor

TEMPLATE = app

SOURCES += monitor.cpp  \
    mainwindow.cpp  \
    embedpython.cpp

HEADERS += mainwindow.h \
    embedpython.h   \
    stdafx.h \
    autotools/clibapp.h

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
include("../lib/QScintilla.pri")

OTHER_FILES += meta.json signal_calc.py \
    autotools/clib*.* \
    autotools/*.h   \
    /Users/hehao/work/lib/libforecaster/*.h \
    conf_linux.json autotools/*.py  \
    ui/*.html \
    ui/js/*.js \

RESOURCES += \
    monitor.qrc

DISTFILES += \
    autotools/__init__.py \
    autotools/builder.py \
    autotools/signal_calc.py \
    autotools/trading_signal.py \
    autotools/market_calc.py \
    autotools/base_ring.py \
    autotools/guava.py \
    autotools/forecasterhandle.py \
    autotools/optimizehandle.py \
    autotools/optimizetask.py \
    autotools/testpy.py \
    ui/js/wsclient.js


