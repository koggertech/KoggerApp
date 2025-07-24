#pragma once

#include <memory>
#include "scene_object.h"
#include "dataset.h"
#include "data_processor.h"


class GraphicsScene3dView;
class BottomTrack : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(BottomTrack)

public:
    enum class ActionEvent {
        Undefined = 0,
        ClearDistProc,
        MaxDistProc,
        MinDistProc
    };
    Q_ENUM(ActionEvent)

    class BottomTrackRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        BottomTrackRenderImplementation();
        virtual ~BottomTrackRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx,
                            const QMatrix4x4& mvp,
                            const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;

        virtual void render(QOpenGLFunctions* ctx,
                            const QMatrix4x4& model,
                            const QMatrix4x4& view,
                            const QMatrix4x4& projection,
                            const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;
    private:
        friend class BottomTrack;
        QVector<int> selectedVertexIndices_;
        bool sideScanUpdated_;
        bool sideScanVisibleState_;
    };

    explicit BottomTrack(GraphicsScene3dView* view = nullptr, QObject* parent = nullptr);
    virtual ~BottomTrack();
    virtual SceneObjectType type() const override;
    virtual bool eventFilter(QObject *watched, QEvent *event) override final;
    void setDatasetPtr(Dataset* datasetPtr);
    void setDataProcessorPtr(DataProcessor* dataProcessorPtr);
    void actionEvent(ActionEvent actionEvent);
    QVector<int> getAllIndxs();
    QVector<int> getRemainingIndxs();

public Q_SLOTS:
    virtual void setData(const QVector<QVector3D>& data, int primitiveType = GL_POINTS) override;
    virtual void clearData() override;
    void isEpochsChanged(int lEpoch, int rEpoch);
    void resetVertexSelection();
    void selectEpoch(int epochIndex, const ChannelId& channelId);
    void sideScanUpdated();
    void setSideScanVisibleState(bool state);
    void setVisibleState(bool state);

Q_SIGNALS:
    void epochHovered(int epochIndex);
    void epochPressed(int epochIndex);
    void epochErased(int epochIndex);
    void epochSelected(int epochIndex, int channelId);
    void epochListChanged();
    void updatedPoints(const QVector<int>& indx);

protected:
    friend class GraphicsScene3dView;

    virtual void mouseMoveEvent(Qt::MouseButtons buttons, qreal x, qreal y) override;
    virtual void mousePressEvent(Qt::MouseButtons buttons, qreal x, qreal y) override;
    virtual void mouseReleaseEvent(Qt::MouseButtons buttons, qreal x, qreal y) override;
    virtual void keyPressEvent(Qt::Key key) override;
    void updateRenderData(bool redrawAll, int lEpoch = 0, int rEpoch = 0);

private:
    QVector<QPair<int, int>> getSubarrays(const QVector<int>& sequenceVector); // TODO: to utils

    using EpochIndex = int;
    using VerticeIndex = int;
    QHash<VerticeIndex,EpochIndex> vertex2Epoch_;
    QHash<VerticeIndex,EpochIndex> epoch2Vertex_;
    QSet<int> allIndxs_;
    DatasetChannel visibleChannel_;
    Dataset* datasetPtr_;
    DataProcessor* dataProcessorPtr_;
    QVector<QVector3D> renderData_;
    int firstLIndx_ = -1;
    int lastIndx_ = -1;
};
