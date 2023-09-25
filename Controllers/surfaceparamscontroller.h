#ifndef SURFACEPARAMSCONTROLLER_H
#define SURFACEPARAMSCONTROLLER_H

#include <QObject>

#include <activeobjectparamsmenucontroller.h>
#include <activeobjectprovider.h>
#include <sceneobjectslistmodel.h>
#include <surface.h>

class SurfaceParamsController : public ActiveObjectParamsMenuController
{
    Q_OBJECT
    Q_PROPERTY(QColor contourColor   READ contourColor     WRITE setContourColor)
    Q_PROPERTY(QColor gridColor      READ gridColor        WRITE setGridColor)
    Q_PROPERTY(bool   contourVisible READ isContourVisible WRITE setContourVisible)
    Q_PROPERTY(bool   gridVisible    READ isGridVisible    WRITE setGridVisible)

public:
    explicit SurfaceParamsController(std::shared_ptr <ActiveObjectProvider> activeObjectProvider,
                                     QObject *parent = nullptr);

    virtual ~SurfaceParamsController();

private:
    void setContourVisible(bool visible);

    void setGridVisible(bool visible);

    void setContourColor(QColor color);

    void setGridColor(QColor color);

    void updateSurface(int  bottomTrackObjectIndex,
                       bool interpolateWithGrid,
                       int  gridCellSize);

    bool isGridVisible() const;

    bool isContourVisible() const;

    QColor gridColor() const;

    QColor contourColor() const;

    Surface* takeSurface();

private:
    std::shared_ptr <ActiveObjectProvider> m_activeObjectProvider;
    std::shared_ptr <SceneObjectsListModel> m_sceneObjectsListModel;

};

#endif // SURFACEPARAMSCONTROLLER_H
