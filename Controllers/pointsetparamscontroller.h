#ifndef POINTSETPARAMSCONTROLLER_H
#define POINTSETPARAMSCONTROLLER_H

#include <QObject>

#include <activeobjectprovider.h>
#include <sceneobjectslistmodel.h>
#include <pointset.h>

class PointSetParamsController : public QObject
{
    Q_OBJECT
public:
    explicit PointSetParamsController(std::shared_ptr<ActiveObjectProvider> activeObjectProvider,
                                      std::shared_ptr<SceneObjectsListModel> sceneObjectsListModel,
                                      QObject *parent = nullptr);

    Q_INVOKABLE void changePointSetVisibility(bool visible);

    Q_INVOKABLE void addPoint(QVector3D point);

    Q_INVOKABLE void removePoint(int index);

    Q_INVOKABLE void changePointCoord(int index, QVector3D coord);

private:
    PointSet* takePointSet();

private:
    std::shared_ptr <ActiveObjectProvider> mActiveObjectProvider;
    std::shared_ptr <SceneObjectsListModel> mSceneObjectsListModel;
};

#endif // POINTSETPARAMSCONTROLLER_H
