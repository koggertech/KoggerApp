#pragma once

#include <stdint.h>
#include <QVector>
#include <QVector3D>

#include "scene_object.h"
#include "surface_view_processor.h"
#include "bottom_track.h"
#include "delaunay.h"


class SurfaceView : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SurfaceView)

public:
    class SurfaceViewRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        SurfaceViewRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx, const QMatrix4x4 &model, const QMatrix4x4 &view, const QMatrix4x4 &projection, const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;

    private:
        friend class SurfaceView;

        QVector<QVector3D> pts_; // для треугольников
        QVector<QVector3D> edgePts_; // для ребер

        float minZ_ = std::numeric_limits<float>::max();
        float maxZ_ = std::numeric_limits<float>::lowest();

        bool trianglesVisible_ = true;
        bool edgesVisible_ = true;
    };

    explicit SurfaceView(QObject* parent = nullptr);
    virtual ~SurfaceView();

    void clear();

    void setBottomTrackPtr(BottomTrack* ptr);

public slots:
    void onUpdatedBottomTrackData(const QVector<int>& indxs);
    void onAction();
    void onTrianglesVisible(bool state) { auto*r=RENDER_IMPL(SurfaceView); r->trianglesVisible_ = state; Q_EMIT changed(); };
    void onEdgesVisible(bool state) { auto*r=RENDER_IMPL(SurfaceView); r->edgesVisible_ = state; Q_EMIT changed(); };

private:
    friend class SurfaceViewProcessor;

    void resetTriangulation();

    delaunay::Delaunay del_;
    BottomTrack* bottomTrackPtr_ = nullptr;
    QHash<int, uint64_t> bTrToTrIndxs_;
    bool originSet_ = false;
    QHash<QPair<int,int>, size_t>  cellPoints_; // fir - virt indx, sec - indx in tr
    int cellPx_ = 1;
    QPointF origin_;
};
