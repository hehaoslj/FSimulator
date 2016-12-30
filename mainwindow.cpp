#include <QGraphicsItem>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTableWidgetItem>
#include <QFile>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>

#include <QGraphicsScene>

LMGraphicsScene::LMGraphicsScene(QObject *parent)
    :QGraphicsScene(parent)
{

}

void LMGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    //qDebug() << "Custom scene clicked.";

    if(!event->isAccepted()) {
        if(event->button() == Qt::LeftButton) {
            blockSignals(true);
            QList<QGraphicsItem *>its = items();
            for(int i=0; i<its.size(); ++i)
            {
                try {
                    QGraphicsTextItem* t = dynamic_cast<QGraphicsTextItem*>(its.at(i));
                    t->setDefaultTextColor(QColor(0, 0, 0));
                } catch(...) {

                }
            }
            blockSignals(false);
            QPointF pt = event->scenePos();
            its= items(pt);
            if(its.size() != 1)
                return;
            try {
                QGraphicsTextItem* t = dynamic_cast<QGraphicsTextItem*>(its.at(0));
                t->setDefaultTextColor(QColor(255, 0, 0));
            } catch(...) {

            }
            emit editItem(its.at(0));
        }
    }
    QGraphicsScene::mousePressEvent(event);
}


inline void get_json_obj(json_t* root, const char* keys, json_t** ret)
{
    json_t* o = root;
    char *key = NULL;
    size_t nsz = strlen(keys) +1;
    char * nkeys = new char[nsz];
    char* last=NULL;
    memcpy(nkeys, keys, nsz);
    nkeys[nsz] = '\0';

    for(key = strtok_r(nkeys, ".", &last);
        key;
        key = strtok_r(NULL, ".", &last))
    {
        if(json_is_object(o))
            o = json_object_get(o, key);
    }

    delete[] nkeys;
    *ret = o;

}

namespace {

template<class T>
static inline void get_json_string(json_t* root ,const char* key, T& value)
{
    json_t *obj = NULL;
    get_json_obj(root, key, &obj);
    if(json_is_string(obj)) {
        value = json_string_value(obj);
    }
}

template<class T>
static inline void get_json_integer(json_t* root, const char* key, T* value)
{
    json_t* obj = NULL;
    get_json_obj(root, key, &obj);
    if(json_is_integer(obj)) {
        *value = json_integer_value(obj);
    }
}

template<class T>
static inline void get_json_float(json_t* root, const char* key, T* value)
{
    json_t* obj = NULL;
    get_json_obj(root, key, &obj);
    if(json_is_real(obj)) {
        *value = json_real_value(obj);
    }
}

static inline void variant_json_value(json_t* o, QVariant& v)
{
    if(json_is_string(o)) {
        v=QString::fromUtf8(json_string_value(o));
    } else if(json_is_boolean(o)) {
        v=QVariant::fromValue(json_boolean_value(o));
    } else if(json_is_integer(o)) {
        v=QVariant::fromValue(json_integer_value(o));
    } else if(json_is_real(o)) {
        v=QVariant::fromValue(json_real_value(o));
    }else if(json_is_null(o)) {
        v=QVariant();
    }else {
        v=QVariant("");
    }
}

static inline void loadMetaData(QTableWidget* table, json_t** pmeta,
                                std::map<QString, std::vector<meta_t> >& m_metaList)
{
    QFile file(":/file/meta.json");
    file.open(QIODevice::ReadOnly);
    QByteArray data = file.readAll();
    file.close();

    std::vector<meta_t> prop;

    json_error_t err;
    json_t* meta;
    meta = json_loads(data.data(), 0, &err);
    void* iter;
    const char *key, *comment;
    const char* key2;
    json_t* obj;
    json_t* obj2;
    json_object_foreach(meta, key, obj) {
        if(json_is_object(obj)) {

            {
                std::vector<meta_t> temp;
                m_metaList.insert(std::pair<QString, std::vector<meta_t> >(key, temp));
            }
            qDebug()<<"key:"<<key;
            comment = NULL;
            iter = json_object_iter_at(obj, "comment");
            obj2 = json_object_iter_value(iter);
            if(json_is_string(obj2)) {
                comment = json_string_value(obj2);
            }
            int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(key));
            table->setItem(row, 1, new QTableWidgetItem(comment));



            json_object_foreach(obj, key2, obj2) {
                meta_t m;
                m.name = key2;
                m.type = json_string_value( json_array_get(obj2, 0) );
                m.desc = json_string_value( json_array_get(obj2, 1) );
                variant_json_value(json_array_get(obj2, 2), m.value);
                m.required = json_boolean_value( json_array_get(obj2, 3) );
                m.group = 1;
                std::map<QString, std::vector<meta_t> >::iterator iter = m_metaList.find(key);
                if(iter != m_metaList.end())
                    iter->second.push_back(m);
            }

        } else {
            meta_t m;
            m.name = key;
            m.type = "string";
            m.group = 0;
            m.desc = json_string_value(obj);
            m.value = m.desc;
            prop.push_back(m);
        }
    }
    for(auto iter = m_metaList.begin(); iter!= m_metaList.end(); ++iter) {
        std::vector<meta_t> &vec = iter->second;
        for(size_t j=0; j<prop.size(); ++j) {

            meta_t& m = prop.at(j);
            bool found = false;
            for(size_t i=0; i< vec.size(); ++i)
            {
                if(vec.at(i).name.compare(m.name) == 0) {
                    vec.at(i).group = m.group;
                    found =true;
                    break;
                }
            }
            if(!found) {
                vec.push_back(m);
            }
        }

        //sort vector
        std::sort(vec.begin(), vec.end(), [](const meta_t& m1, const meta_t&m2)
        {
            return m1.name < m2.name;
        });
    }

    table->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    //table->setDisabled(true);

    *pmeta = meta;
}

static inline bool meta_vec_contain(std::vector<meta_t>& vec, const QString& name)
{
    for(size_t i=0; i<vec.size(); ++i)
    {
        auto m = vec.at(i);
        if(m.name.compare(name) == 0)
            return true;
    }
    return false;
}

static inline void getMetaConfig(json_t* meta, const char* type, std::vector<meta_t>& value)
{
    const char* key;
    json_t* obj;
    json_t* obj2 = NULL;
    json_object_foreach(meta, key, obj)
    {
        if(json_is_object(obj))
        {
            if(strcmp(key, type) != 0)
            {
                continue;
            }
            obj2 = obj;
        } else {
            meta_t m;
            m.name = key;
            m.type = "string";
            m.desc = json_string_value(obj);
            m.required = false;
            m.value = m.desc;

            m.group = 0;
            if(!meta_vec_contain(value, m.name))
                value.push_back(m);
        }
    }

    const char* key2;
    obj = obj2;
    json_object_foreach(obj, key2, obj2)
    {
        if(json_is_object(obj2))
        {
            continue;
        }
        meta_t m;
        m.name = key2;
        m.type = json_string_value(  json_array_get(obj2, 0) );
        m.desc = json_string_value(  json_array_get(obj2, 1) );
        m.required =  json_boolean_value(  json_array_get(obj2, 3) );
        json_t* o = json_array_get(obj2, 2);
        if( json_is_string(o) ) {
            m.value = QString(json_string_value(o));
        } else if(json_is_integer(o)) {
            m.value = QVariant::fromValue(json_integer_value(o));
        } else if(json_is_real(o)) {
            m.value = QVariant::fromValue(json_real_value(o));
        }
        m.group = 1;
        for(size_t i=0; i< value.size(); ++i)
        {
            if(value.at(i).name.compare(m.name) == 0) {
                m.group = value.at(i).group;

                value.erase(value.begin()+i);
                break;
            }
        }
        value.push_back(m);
    }

    std::sort(value.begin(), value.end(), [](const meta_t& m1, const meta_t&m2)
    {
        return m1.name < m2.name;
    });
}

static inline void updateProp(json_t* meta, const QString& type,
                              std::vector<meta_t>& vec,
                              std::map<QString, QtVariantProperty*>& m_propList,
                              std::map<QtProperty*, meta_t> m_ppidList,
                              QtVariantPropertyManager *m_propManager,
                              QtTreePropertyBrowser* propBrowser)
{
    //Find meta info
    //std::vector<meta_t> vec;
    //getMetaConfig(meta, type.toUtf8().data(), vec);

    for(auto iter = m_ppidList.begin(); iter != m_ppidList.end(); ++iter) {
        delete iter->first;
    }
    m_propList.clear();
    m_ppidList.clear();
    propBrowser->clear();
    m_propManager->clear();
    QtProperty *topItem[2];
    topItem[0] = m_propManager->addProperty(QtVariantPropertyManager::groupTypeId(),
                QString::number(1) + QString(" General Property"));
    topItem[1] = m_propManager->addProperty(QtVariantPropertyManager::groupTypeId(),
                QString::number(2) + QString(" %1 Property").arg(type));

    for(size_t i=0; i<vec.size(); ++i) {
        meta_t& m = vec.at(i);
        QtVariantProperty *prop;
        if(m.type.compare("date") == 0) {
            prop = m_propManager->addProperty(QVariant::Date, m.name);
        }
        else if(m.type.compare("time") == 0) {
            prop = m_propManager->addProperty(QVariant::Time, m.name);
        }
        else if(m.type.compare("integer") == 0) {
            prop = m_propManager->addProperty(QVariant::Int, m.name);
        }
        else {
            prop = m_propManager->addProperty(QVariant::String, m.name);
        }
        prop->setValue(m.value);
        if(m.name.compare("type") == 0) {
            prop->setEnabled(false);
            prop->setValue(type);

        }

        topItem[m.group % 2]->addSubProperty(prop);

        m_propList[m.name] = prop;
        m_ppidList[prop] = m;
    }
    propBrowser->addProperty(topItem[0]);
    propBrowser->addProperty(topItem[1]);
}

}
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    loadMetaData(ui->metaTable, &meta, m_metaList);
    ui->summaryTable->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    ui->summaryTable->setSelectionMode(QAbstractItemView::SingleSelection);

    m_propManager= new QtVariantPropertyManager(this);
    m_propFactory = new QtVariantEditorFactory(this);
    ui->props->setFactoryForManager(m_propManager, m_propFactory);

    //QtVariantProperty *item = m_propManager->addProperty(QVariant::Bool, QString::number(1) + QLatin1String(" Bool Property"));
    //ui->props->addProperty(item);
    //variantEditor->setPropertiesWithoutValueMarked(true);
    //variantEditor->setRootIsDecorated(false);

    scene = new LMGraphicsScene();
    ui->graphicsView->setScene(scene);

    connect(scene, SIGNAL(editItem(QGraphicsItem*)),
            this, SLOT(handle_edit_graphicsitem(QGraphicsItem*)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_action_New_triggered()
{
    int row = ui->metaTable->currentRow();
    if(row <0)
        return;
    QString type = ui->metaTable->item(row, 0)->text();
    row = ui->summaryTable->rowCount();
    ui->summaryTable->insertRow(row);
    ui->summaryTable->setItem(row, 0, new QTableWidgetItem("name"));
    ui->summaryTable->setItem(row, 1, new QTableWidgetItem(type));

    updateProp(meta, type, m_metaList[type], m_propList, m_ppidList, m_propManager, ui->props);

    QGraphicsTextItem* item = scene->addText(type);
    m_gitmList[item] = type;
    item->setPos(m_gitmList.size()*100, 0);
    item->setCursor(QCursor(Qt::PointingHandCursor));
    //item->setTextInteractionFlags(Qt::TextSelectableByMouse);
    item->setFlag(QGraphicsItem::ItemIsMovable, true);
}

void MainWindow::on_actionDelete_triggered()
{
    ui->summaryTable->removeRow(ui->summaryTable->currentRow());
}

void MainWindow::on_summaryTable_clicked(const QModelIndex &index)
{
    QString type = ui->summaryTable->item(index.row(), 1)->text();

    updateProp(meta, type, m_metaList[type], m_propList, m_ppidList, m_propManager, ui->props);
}

void MainWindow::handle_edit_graphicsitem(QGraphicsItem* item)
{
    std::map<QGraphicsItem*, QString>::const_iterator iter = m_gitmList.find(item);
    if(iter != m_gitmList.end())
    {
        updateProp(meta, iter->second,m_metaList[iter->second],  m_propList, m_ppidList, m_propManager, ui->props);
    }
}

void MainWindow::on_actionRun_triggered()
{

}
