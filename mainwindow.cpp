#include <QGraphicsItem>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTableWidgetItem>
#include <QFile>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>

#include <QGraphicsScene>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QWebSettings* webSetting = QWebSettings::globalSettings();
    webSetting->setAttribute(QWebSettings::JavascriptEnabled, true);
    webSetting->setAttribute(QWebSettings::PluginsEnabled, true);
    web = new QWebView(ui->centralwidget);
    QString url = "file:///Users/hehao/work/lib/metroui/docs/index.html";
    //QString url = "file:///Users/hehao/work/lib/bootstrap/docs/index.html";
    url="http://192.168.56.101:8801/en/recruitment";
    web->load(url);

    connect(web, SIGNAL(titleChanged(QString)), this, SLOT(setTitle(QString)));


}

MainWindow::~MainWindow()
{
    delete web;
    delete ui;
}

void MainWindow::setTitle(const QString &title)
{
    setWindowTitle(title);
}


void MainWindow::resizeEvent(QResizeEvent *event)
{
    (void)event;
    web->resize( ui->centralwidget->size() );

}


