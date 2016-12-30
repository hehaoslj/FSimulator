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

class LMGraphicsScene:public QGraphicsScene
{
    Q_OBJECT
public:
    LMGraphicsScene(QObject *parent = Q_NULLPTR);
signals:
    void editItem(QGraphicsItem* item);
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
};

namespace Ui {
class MainWindow;
}

struct meta_t {
    QString name;
    QString type;
    QString desc;
    QVariant value;
    bool    required;
    int group;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void on_action_New_triggered();

    void on_actionDelete_triggered();

    void on_summaryTable_clicked(const QModelIndex &index);

    void handle_edit_graphicsitem(QGraphicsItem* item);
private slots:
    void on_actionRun_triggered();

private:
    Ui::MainWindow *ui;

    LMGraphicsScene* scene;
    std::map<QGraphicsItem*, QString> m_gitmList;

    json_t* meta;
    json_t* m_conf;
    QString m_confFile;

    std::map<QString, std::vector<meta_t> >m_metaList;

    QtVariantPropertyManager *m_propManager;
    QtVariantEditorFactory *m_propFactory;

    std::map<QString, QtVariantProperty*> m_propList;
    std::map<QtProperty*, meta_t> m_ppidList;
};

#endif // MAINWINDOW_H
