#include "variantfilebrowsemanager.h"

class VariantFileBrowseType
{
};
Q_DECLARE_METATYPE(VariantFileBrowseType)



VariantFileBrowseManager::VariantFileBrowseManager(QObject *parent):
    QtVariantPropertyManager(parent)
{

}

VariantFileBrowseManager::~VariantFileBrowseManager()
{

}

QVariant VariantFileBrowseManager::value(const QtProperty *property) const
{
    auto iter = theValues.find(property);
    if(iter == theValues.end()) {
        return QVariant();
    }
    return iter->value;
}

int VariantFileBrowseManager::valueType(int propertyType) const
{
    if (propertyType == qMetaTypeId<VariantFileBrowseType>())
        return  QVariant::String;
    return QtVariantPropertyManager::
        isPropertyTypeSupported(propertyType);

}

bool VariantFileBrowseManager::isPropertyTypeSupported(int propertyType) const
{
    if (propertyType == qMetaTypeId<VariantFileBrowseType>())
        return true;
    return QtVariantPropertyManager::
        isPropertyTypeSupported(propertyType);
}

void VariantFileBrowseManager::setValue(QtProperty *p, const QVariant &val)
{
    auto iter = theValues.find(p);
    if(iter == theValues.end()) {
        return;
    }
    iter->value = val.toString();
}

void VariantFileBrowseManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
    auto iter = theValues.find(property);
    if(iter == theValues.end()) {
        return;
    }
    if(attribute.compare("filter") == 0)
    {
        iter->filter = value.toString();
    }

}
