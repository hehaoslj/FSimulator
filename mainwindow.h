#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <map>
#include <vector>
#include <QString>

#include <QMainWindow>


#include <jansson.h>
#include "qtvariantproperty.h"

#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>

#include <QtWebKitWidgets>


namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:

    void setTitle(const QString& title);

private slots:
    void resizeEvent(QResizeEvent *event);


private:
    Ui::MainWindow *ui;

    QWebView* web;
};

#endif // MAINWINDOW_H
