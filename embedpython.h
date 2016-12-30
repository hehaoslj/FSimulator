#ifndef EMBEDPYTHON_H
#define EMBEDPYTHON_H

#include <stdarg.h>
#include <string>

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QMutex>
#include <python.h>

class EmbedPython:public QObject
{
    Q_OBJECT
public:
    enum cmd_type{SINGLE_LINE=256, MULTI_LINE=257};
    EmbedPython(QObject* parent = 0);
    ~EmbedPython();

    bool init();
    void finit();
    void reload();

    int exec(const char* cmd, int type = SINGLE_LINE);
    int callModel(const char *method, const char *format = NULL, ...);
    int reloadModel(const char* model);
    const char* returnType() const;
    PyObject* returnObject() const;
    QString errorMessage() const;
    void checkError();

signals:
    void errorTrigger(const QString&);
    void outputTrigger(const QString&);
private:
    QByteArray  py_progname;
    QByteArray py_pythonhome;
    PyObject *modeltool;
    //PyObject *mainmodel;
    PyObject *retobj;
    PyObject *globals;
    //PyObject *locals;
    bool initialized;
    QMutex mutex;
    QString emsg;

};

#endif // EMBEDPYTHON_H
