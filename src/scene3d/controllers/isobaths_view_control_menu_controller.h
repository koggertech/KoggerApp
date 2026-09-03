#pragma once

#include <functional>
#include <QThread>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include "qml_component_controller.h"
#include "data_processor.h"


class GraphicsScene3dView;
class IsobathsViewControlMenuController : public QmlComponentController
{
    Q_OBJECT

    Q_PROPERTY(QVariantMap pipelineStatus READ pipelineStatus NOTIFY pipelineStatusChanged)
    Q_PROPERTY(bool statusMonitorEnabled READ statusMonitorEnabled WRITE setStatusMonitorEnabled NOTIFY statusMonitorEnabledChanged)
    Q_PROPERTY(bool statusDetailedPolling READ statusDetailedPolling WRITE setStatusDetailedPolling NOTIFY statusDetailedPollingChanged)

public:
    explicit IsobathsViewControlMenuController(QObject* parent = nullptr);

    void setGraphicsSceneView(GraphicsScene3dView* sceneView);
    void setDataProcessorPtr(DataProcessor *dataProcessorPtr);

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
    // Colormap gradient stops [{pos, color}] for the theme swatch (QML picker).
    Q_INVOKABLE QVariantList themeStops(int index) const;

    QVariantMap pipelineStatus() const;
    bool statusMonitorEnabled() const;
    void setStatusMonitorEnabled(bool state);
    bool statusDetailedPolling() const;
    void setStatusDetailedPolling(bool state);
    Q_INVOKABLE void refreshPipelineStatus();

signals:
    void pipelineStatusChanged();
    void statusMonitorEnabledChanged();
    void statusDetailedPollingChanged();

protected:
    void findComponent() override;

private slots:
    void onPipelineStats(const QVariantMap& stats);

private:
    void tryInitPendingLambda();

private:
    GraphicsScene3dView* graphicsSceneViewPtr_;
    DataProcessor* dataProcessorPtr_;
    std::function<void()> pendingLambda_;
    QThread thread_;
    QTimer statusTimer_;
    QVariantMap pipelineStatus_;
    float surfaceLineStepSize_;
    int themeId_;
    int labelStepSize_;
    int edgeLimit_;
    int extraWidth_;
    bool visibility_;
    bool edgesVisible_;
    bool trianglesVisible_;
    bool debugModeView_;
    bool processState_;
    bool statusMonitorEnabled_;
    bool statusRequestPending_;
    bool statusDetailedPolling_;
};
