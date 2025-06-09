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
    struct ColorInterval
    {
        float depth = 0.0f;
        QVector3D color;
        ColorInterval() = default;
        ColorInterval(float d, const QVector3D &c) : depth(d), color(c) {}
    };

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

        /*data*/
        QVector<ColorInterval> colorIntervals_;
        float levelStep_ = 3.0f;
        float lineStepSize_ = 3.0f;
        GLuint textureId_ = 0;
        //QVector<QVector3D> lineSegments_;
        QVector3D color_;
    };

    explicit SurfaceView(QObject* parent = nullptr);
    virtual ~SurfaceView();
    void clear();
    void setBottomTrackPtr(BottomTrack* ptr);

    QVector<uint8_t>& getTextureTasksRef();
    GLuint getDeinitTextureTask() const;
    GLuint getTextureId() const;
    void setTextureId(GLuint textureId);
    void setColorTableThemeById(int id);

    float getSurfaceStepSize() const;
    void setSurfaceStepSize(float val);
    float getLineStepSize() const;
    void setLineStepSize(float val);

public slots:
    void onUpdatedBottomTrackData(const QVector<int>& indxs);
    void onAction();
    void onTrianglesVisible(bool state) { auto*r=RENDER_IMPL(SurfaceView); r->trianglesVisible_ = state; Q_EMIT changed(); };
    void onEdgesVisible(bool state) { auto*r=RENDER_IMPL(SurfaceView); r->edgesVisible_ = state; Q_EMIT changed(); };

private:
    friend class SurfaceViewProcessor;

    void resetTriangulation();
    void rebuildColorIntervals();
    QVector<QVector3D> generateExpandedPalette(int totalColors) const;
    void updateTexture();

    delaunay::Delaunay del_;
    BottomTrack* bottomTrackPtr_ = nullptr;
    QHash<int, uint64_t> bTrToTrIndxs_;
    bool originSet_ = false;
    QHash<QPair<int,int>, QVector3D>  cellPoints_; // fir - virt indx, sec - indx in tr
    QHash<QPair<int,int>, int>  cellPointsInTri_;
    QPair<int,int> lastCellPoint_;
    int cellPx_ = 1;
    QPointF origin_;

    /*data*/
    float minDepth_ = 0.0f;
    float maxDepth_ = 0.0f;
    float surfaceStepSize_ = 1.0f;
    float lineStepSize_    = 1.0f;
    float labelStepSize_   = 100.0f;
    GLuint textureId_ = 0;
    QVector<uint8_t> textureTask_;
    GLuint toDeleteId_ = 0;
    int themeId_ = 0;
};
