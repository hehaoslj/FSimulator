#ifndef STDAFX_H
#define STDAFX_H
//#include <python2.7/Python.h>
//#include <python2.7/frameobject.h>
#include <Python.h>
#include <frameobject.h>

#include <Qsci/qscilexercpp.h>
#include <Qsci/qscilexerxml.h>
#include <Qsci/qscilexerpython.h>

//Qt Library
#include <QMessageBox>
#include <QApplication>
#include <QString>
#include <QDir>
#include <QDebug>
#include <QByteArray>
#include <QActionGroup>
#include <QFileIconProvider>

#include <QMutexLocker>
#include <QMutex>

//STD library
#include <vector>
#include <string>

//Platform specs
#ifdef _WIN32
#define WIN32_MEAN_AND_LEAN
#include <windows.h>
#include <Shellapi.h>
#pragma comment(lib, "Shell32.lib")
#endif


#endif /** STDAFX_H */
