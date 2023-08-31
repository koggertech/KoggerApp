#include "polygongroup.h"

PolygonGroup::PolygonGroup(QObject *parent)
    : SceneObject(parent)
    , mModel(new QStandardItemModel)
{}

QStringList PolygonGroup::visualItems() const
{
    QStringList result;

    for(const auto& polygon : mPolygonList)
        result.append(polygon->name());

    return result;
}

PolygonObject *PolygonGroup::polygonAt(int index) const
{
    return at(index).get();
}

void PolygonGroup::draw(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, QMap<QString, QOpenGLShaderProgram *> shaderProgramMap) const
{
    for(auto polygon : mPolygonList)
        polygon->draw(ctx, mvp, shaderProgramMap);
}

SceneObject::SceneObjectType PolygonGroup::type() const
{
    return SceneObject::SceneObjectType::PolygonGroup;
}

void PolygonGroup::addPolygon(std::shared_ptr <PolygonObject> polygon)
{
    mPolygonList.append(polygon);

    auto item = new QStandardItem();
    item->setData(polygon->name(), Qt::DisplayRole);

    mModel->invisibleRootItem()->appendRow(item);

    Q_EMIT countChanged(mPolygonList.count());
}

void PolygonGroup::removePolygon(int index)
{
    if(index < 0 && index >= mPolygonList.count())
        return;

    mPolygonList.removeAt(index);

    Q_EMIT countChanged(mPolygonList.count());

    mModel->invisibleRootItem()->removeRow(index);
}

PolygonPtr PolygonGroup::at(int index) const
{
    if(index < 0 || index >= mPolygonList.count())
        return nullptr;

    for(int i = 0; i < mPolygonList.count(); i++){
        if(i == index)
            return mPolygonList[i];
    }

    return nullptr;
}

int PolygonGroup::polygonsCount() const
{
    return mPolygonList.count();
}

int PolygonGroup::indexOf(std::shared_ptr<PolygonObject> polygon) const
{
    if(polygon)
        return mPolygonList.indexOf(polygon);

    return 0;
}


QStandardItemModel *PolygonGroup::model() const
{
    return mModel.get();
}

