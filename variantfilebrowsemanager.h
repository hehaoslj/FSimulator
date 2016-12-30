#ifndef VARIANTFILEBROWSEMANAGER_H
#define VARIANTFILEBROWSEMANAGER_H
#include "qtvariantproperty.h"


class VariantFileBrowseManager : public QtVariantPropertyManager
{
    Q_OBJECT
public:
    VariantFileBrowseManager(QObject *parent = 0);
    virtual ~VariantFileBrowseManager();

    QVariant value(const QtProperty *property) const;
    int valueType(int propertyType) const;
    bool isPropertyTypeSupported(int propertyType) const;

public slots:
    void setValue(QtProperty *p, const QVariant &val);
    void setAttribute(QtProperty *property,
                    const QString &attribute, const QVariant &value);
protected:

private slots:

private:
    struct Data {
        QString value;
        QString filter;
    };
    QMap<const QtProperty *, Data> theValues;
};

#endif // VARIANTFILEBROWSEMANAGER_H
