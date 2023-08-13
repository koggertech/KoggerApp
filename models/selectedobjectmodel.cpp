#include "selectedobjectmodel.h"

SelectedObjectModel::SelectedObjectModel(QObject *parent)
    : QObject{parent}
{}

QVariantMap SelectedObjectModel::objectParams() const
{
    return mpObject->params();
}

SelectedObjectModel::SelectedObjectModel(std::shared_ptr<VertexObject> object, QObject *parent)
: QObject{parent}
, mpObject(object)
{}

void SelectedObjectModel::setObject(std::shared_ptr <VertexObject> object)
{
    mpObject = object;

    Q_EMIT objectChanged();
}

std::shared_ptr<VertexObject> SelectedObjectModel::object() const
{
    return mpObject;
}
