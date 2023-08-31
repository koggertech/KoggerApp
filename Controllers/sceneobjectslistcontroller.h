#ifndef SCENEOBJECTSLISTCONTROLLER_H
#define SCENEOBJECTSLISTCONTROLLER_H

#include <QObject>
#include <QStandardItemModel>

#include <sceneobjectslistmodel.h>
#include <activeobjectprovider.h>
#include <bottomtrackprovider.h>

class Scene3D;

class SceneObjectsListController : public QObject
{
    Q_OBJECT

public:
    explicit SceneObjectsListController(
                std::shared_ptr <QStandardItemModel> sceneItemListModel,
                std::shared_ptr <BottomTrackProvider> bottomTrackProvider,

                QObject *parent = nullptr
            );

    virtual ~SceneObjectsListController();

    Q_INVOKABLE void addObject(QString name, SceneObject::SceneObjectType type);

    Q_INVOKABLE void removeObject(int index);

    Q_INVOKABLE void setObjectName(int index, QString name);

    Q_INVOKABLE void setCurrentObject(int index);

private:

    std::shared_ptr <SceneObjectsListModel> mpModel;
    std::shared_ptr <ActiveObjectProvider> mpActiveObjectProvider;
    std::shared_ptr <BottomTrackProvider> mpBottomTrackProvider;
};

#endif // SCENEOBJECTSLISTCONTROLLER_H
