#ifndef SURFACEPARAMSCONTROLLER_H
#define SURFACEPARAMSCONTROLLER_H

#include <QObject>

#include <activeobjectprovider.h>
#include <sceneobjectslistmodel.h>
#include <surface.h>

class SurfaceParamsController : public QObject
{
    Q_OBJECT
public:
    explicit SurfaceParamsController(std::shared_ptr <ActiveObjectProvider> activeObjectProvider,
                                     std::shared_ptr <SceneObjectsListModel> sceneObjectsListModel,
                                     QObject *parent = nullptr);

    virtual ~SurfaceParamsController();

    Q_INVOKABLE void changeSurfaceVisibility(bool visible);

    Q_INVOKABLE void changeSurfaceContourVisibility(bool visible);

    Q_INVOKABLE void changeSurfaceGridVisibility(bool visible);

    Q_INVOKABLE void changeSurfaceContourColor(QColor color);

    Q_INVOKABLE void changeSurfaceGridColor(QColor color);

    Q_INVOKABLE void updateSurface(int  bottomTrackObjectIndex,
                                   bool interpolateWithGrid,
                                   int  gridCellSize);

private:

    Surface* takeSurface();

private:

    std::shared_ptr <ActiveObjectProvider> mActiveObjectProvider;
    std::shared_ptr <SceneObjectsListModel> mSceneObjectsListModel;

};

#endif // SURFACEPARAMSCONTROLLER_H
