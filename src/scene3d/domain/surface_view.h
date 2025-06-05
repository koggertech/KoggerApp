#pragma once

#include <stdint.h>
#include <QVector>
#include <QVector3D>

#include "dataset.h"
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
    };

    explicit SurfaceView(QObject* parent = nullptr);
    virtual ~SurfaceView();

    void setBottomTrackPtr(BottomTrack* ptr);

public slots:
    void onUpdatedBottomTrackData(const QVector<int>& indxs);

private:
    friend class SurfaceViewProcessor;

    delaunay::Delaunay del_;
    BottomTrack* bottomTrackPtr_ = nullptr;

    QHash<int, uint64_t> bTrToTrIndxs_;
};
