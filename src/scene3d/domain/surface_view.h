#pragma once

#include <stdint.h>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QHash>
#include <QMutex>
#include <QSet>
#include <QVector>
#include <QVector3D>
#include "bottom_track.h"
#include "delaunay.h"
#include "isobaths_defs.h"
#include "scene_object.h"


using namespace IsobathUtils;

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

        // data
        QVector<QVector3D> pts_; // для треугольников
        QVector<QVector3D> edgePts_; // для ребер
        float minZ_;
        float maxZ_;
        bool trianglesVisible_;
        bool edgesVisible_;
        QVector<ColorInterval> colorIntervals_;
        float levelStep_;
        float lineStepSize_;
        GLuint textureId_;
        QVector<QVector3D> lineSegments_;
        QVector<LLabelInfo> labels_;
        QVector3D color_;
        float distToFocusPoint_;
        bool debugMode_;
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
    float getLabelStepSize() const;
    void setLabelStepSize(float val);
    void setCameraDistToFocusPoint(float val);
    void setDebugMode(bool state) { auto*r=RENDER_IMPL(SurfaceView); r->debugMode_ = state; Q_EMIT changed(); };
    void onTrianglesVisible(bool state) { auto*r=RENDER_IMPL(SurfaceView); r->trianglesVisible_ = state; Q_EMIT changed(); };
    void onEdgesVisible(bool state) { auto*r=RENDER_IMPL(SurfaceView); r->edgesVisible_ = state; Q_EMIT changed(); };
    void onProcessStateChanged(bool state) { processState_ = state; };
    bool processState() const { return processState_; };
    void setEdgeLimit(int val);
    void setHandleXCall(int val);

public slots:
    void onAction();
    void onUpdatedBottomTrackDataWrapper(const QVector<int>& indxs);

private slots:
    void handleWorkerFinished();

private:
    // methods
    void onUpdatedBottomTrackData(const QVector<int>& indxs);
    void rebuildColorIntervals();
    QVector<QVector3D> generateExpandedPalette(int totalColors) const;
    void updateTexture();
    void fullRebuildLinesLabels();
    void incrementalProcessLinesLabels(const QSet<int>& updsTrIndx);
    QVector<QVector3D> buildGridTriangles(const QVector<QVector3D>& pts, int gridWidth, int gridHeight) const;
    void buildPolylines(const IsobathsSegVec& segs, IsobathsPolylines& polylines) const;
    void edgeIntersection(const QVector3D& vertA, const QVector3D& vertB, float level, QVector<QVector3D>& out) const;
    void filterNearbyLabels(const QVector<LLabelInfo>& inputData, QVector<LLabelInfo>& outputData) const;
    void filterLinesBehindLabels(const QVector<LLabelInfo>& filteredLabels, const QVector<QVector3D>& inputData, QVector<QVector3D>& outputData) const;
    void enqueueWork(const QVector<int>& indxs, bool rebuildLinesLabels);

    // data
    delaunay::Delaunay del_;
    BottomTrack* bottomTrackPtr_ = nullptr;
    QHash<int, uint64_t> bTrToTrIndxs_;
    bool originSet_ = false;
    QHash<QPair<int,int>, QVector3D>  cellPoints_; // fir - virt indx, sec - indx in tr
    QHash<QPair<int,int>, int>  cellPointsInTri_;
    QPair<int,int> lastCellPoint_;
    int cellPx_;
    QPointF origin_;
    float surfaceStepSize_;
    float lineStepSize_;
    float labelStepSize_;
    GLuint textureId_;
    QVector<uint8_t> textureTask_;
    GLuint toDeleteId_;
    int themeId_;
    bool processState_;
    float edgeLimit_;
    QHash<uint64_t, QVector<int>> pointToTris_;
    IsoState isoState_;
    uint8_t updCnt_;
    uint8_t handleXCall_;
    QFuture<void> workerFuture_;
    QFutureWatcher<void> workerWatcher_;
    QMutex pendingMtx_;
    QVector<int> pendingIndxs_;
    PendingWork pending_;
};
