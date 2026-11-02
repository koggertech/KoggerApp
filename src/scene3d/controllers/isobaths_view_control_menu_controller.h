#pragma once

#include <functional>
#include <QFutureWatcher>
#include <QThread>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include "qml_component_controller.h"
#include "data_processor.h"
#include "surface_grid_export.h"


class Dataset;
class GraphicsScene3dView;
class IsobathsViewControlMenuController : public QmlComponentController
{
    Q_OBJECT

    Q_PROPERTY(QVariantMap pipelineStatus READ pipelineStatus NOTIFY pipelineStatusChanged)
    Q_PROPERTY(int pipelineStatusSeq READ pipelineStatusSeq NOTIFY pipelineStatusSeqChanged)
    Q_PROPERTY(bool statusMonitorEnabled READ statusMonitorEnabled WRITE setStatusMonitorEnabled NOTIFY statusMonitorEnabledChanged)
    Q_PROPERTY(bool statusDetailedPolling READ statusDetailedPolling WRITE setStatusDetailedPolling NOTIFY statusDetailedPollingChanged)
    Q_PROPERTY(float isobathStep READ isobathStep NOTIFY isobathStepChanged)
    Q_PROPERTY(bool bandedColors READ bandedColors NOTIFY bandedColorsChanged)
    Q_PROPERTY(QVariantList surfacePaletteColors READ surfacePaletteColors NOTIFY surfacePaletteColorsChanged)
    Q_PROPERTY(float surfaceDepthMin READ surfaceDepthMin NOTIFY surfaceDepthRangeChanged)
    Q_PROPERTY(float surfaceDepthMax READ surfaceDepthMax NOTIFY surfaceDepthRangeChanged)
    Q_PROPERTY(bool surfaceExportRunning READ surfaceExportRunning NOTIFY surfaceExportRunningChanged)
    Q_PROPERTY(QVariantMap surfaceExportEstimate READ surfaceExportEstimate NOTIFY surfaceExportEstimateChanged)
    Q_PROPERTY(QVariantMap surfaceExportLastResult READ surfaceExportLastResult NOTIFY surfaceExportLastResultChanged)
    Q_PROPERTY(bool surfaceExportEstimateActive READ surfaceExportEstimateActive WRITE setSurfaceExportEstimateActive NOTIFY surfaceExportEstimateActiveChanged)

public:
    explicit IsobathsViewControlMenuController(QObject* parent = nullptr);
    ~IsobathsViewControlMenuController() override;

    void setGraphicsSceneView(GraphicsScene3dView* sceneView);
    void setDataProcessorPtr(DataProcessor *dataProcessorPtr);
    void setDatasetPtr(Dataset* datasetPtr);

    Q_INVOKABLE void onIsobathsVisibilityCheckBoxCheckedChanged(bool checked);
    Q_INVOKABLE void onUpdateIsobathsButtonClicked();
    Q_INVOKABLE void onTrianglesVisible(bool state);
    Q_INVOKABLE void onEdgesVisible(bool state);
    Q_INVOKABLE void onSetSurfaceLineStepSize(float val);
    Q_INVOKABLE void onSetLabelStepSize(int val);
    Q_INVOKABLE void onThemeChanged(int val);
    Q_INVOKABLE void onDebugModeView(bool state);
    Q_INVOKABLE void onProcessStateChanged(bool state);
    Q_INVOKABLE void onResetIsobathsButtonClicked();
    Q_INVOKABLE void onEdgeLimitChanged(int val);
    Q_INVOKABLE void onSetExtraWidth(int val);
    Q_INVOKABLE void setBandedColors(bool state);
    // Colormap gradient stops [{pos, color}] for the theme swatch (QML picker).
    Q_INVOKABLE QVariantList themeStops(int index) const;

    QVariantMap pipelineStatus() const;
    bool statusMonitorEnabled() const;
    void setStatusMonitorEnabled(bool state);
    bool statusDetailedPolling() const;
    void setStatusDetailedPolling(bool state);
    Q_INVOKABLE void refreshPipelineStatus();
    int pipelineStatusSeq() const { return pipelineStatusSeq_; }

    float isobathStep() const;
    bool bandedColors() const;
    QVariantList surfacePaletteColors() const;
    float surfaceDepthMin() const;
    float surfaceDepthMax() const;

    Q_INVOKABLE bool startSurfaceExport(const QString& path, int quality, bool extrapolation, double depthOffset, int format = 0);
    Q_INVOKABLE void cancelSurfaceExport();
    Q_INVOKABLE void requestSurfaceExportEstimate();
    Q_INVOKABLE double surfaceExportNodeStep(int quality) const;
    bool surfaceExportRunning() const;
    QVariantMap surfaceExportEstimate() const;
    QVariantMap surfaceExportLastResult() const;
    bool surfaceExportEstimateActive() const;
    void setSurfaceExportEstimateActive(bool state);

signals:
    void pipelineStatusChanged();
    void pipelineStatusSeqChanged();
    void statusMonitorEnabledChanged();
    void statusDetailedPollingChanged();
    void isobathStepChanged();
    void bandedColorsChanged();
    void surfacePaletteColorsChanged();
    void surfaceDepthRangeChanged();
    void surfaceExportRunningChanged();
    void surfaceExportEstimateChanged();
    void surfaceExportLastResultChanged();
    void surfaceExportEstimateActiveChanged();

protected:
    void findComponent() override;

private slots:
    void onPipelineStats(const QVariantMap& stats);
    void onSurfaceMinZ(float minZ);
    void onSurfaceMaxZ(float maxZ);
    void onSurfaceCleared();
    void onSurfaceExportSnapshotReady(int requestId, SurfaceExportSnapshotPtr snapshot);
    void onSurfaceExportProgress(int value);
    void onSurfaceExportFinished();
    void onSurfaceExportEstimateFinished();
    void onProcessingIdleChanged(bool idle);
    void onDataProcessorDestroyed();

private:
    void tryInitPendingLambda();
    void setSurfaceExportRunning(bool state);
    void finishSurfaceExport(const SurfaceExportResult& result);
    bool requestSurfaceExportSnapshot(int requestId, quint64 knownRevision);
    QVector<QVector3D> bottomTrackData() const;
    void resetSurfaceExportEstimate();

private:
    GraphicsScene3dView* graphicsSceneViewPtr_;
    DataProcessor* dataProcessorPtr_;
    Dataset* datasetPtr_;
    QFutureWatcher<SurfaceExportResult> exportWatcher_;
    QFutureWatcher<SurfaceExportResult> estimateWatcher_;
    QTimer estimateRefreshTimer_;
    SurfaceExportParams exportParams_;
    std::shared_ptr<std::atomic_bool> exportCancel_;
    QVariantMap exportEstimate_;
    quint64 estimateRevision_ = 0;
    qsizetype estimateTrackSize_ = -1;
    QVariantMap exportLastResult_;
    int exportRequestCounter_ = 0;
    int exportRequestId_ = 0;
    int estimateRequestId_ = 0;
    int exportPercent_ = -1;
    bool exportRunning_ = false;
    bool estimatePending_ = false;
    bool estimateActive_ = false;
    bool processingIdle_ = true;
    std::function<void()> pendingLambda_;
    QThread thread_;
    QTimer statusTimer_;
    QVariantMap pipelineStatus_;
    int pipelineStatusSeq_ = 0;
    bool edgeLimitApplied_ = false;
    bool surfaceLineStepApplied_ = false;
    bool extraWidthApplied_ = false;
    float surfaceDepthMin_;
    float surfaceDepthMax_;
    float surfaceLineStepSize_;
    int themeId_;
    int labelStepSize_;
    int edgeLimit_;
    int extraWidth_;
    bool visibility_;
    bool bandedColors_;
    bool edgesVisible_;
    bool trianglesVisible_;
    bool debugModeView_;
    bool processState_;
    bool statusMonitorEnabled_;
    bool statusRequestPending_;
    bool statusDetailedPolling_;
};
