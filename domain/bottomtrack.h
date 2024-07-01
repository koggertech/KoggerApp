#pragma once

#include <memory>
#include "sceneobject.h"
#include "plotcash.h"


class GraphicsScene3dView;
class Surface;
class BottomTrack : public SceneObject
{
    Q_OBJECT
    Q_PROPERTY(DatasetChannel visibleChannel READ visibleChannel WRITE setVisibleChannel NOTIFY visibleChannelChanged FINAL)
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
        bool surfaceUpdated_;
        bool surfaceState_;
    };

    explicit BottomTrack(GraphicsScene3dView* view = nullptr, QObject* parent = nullptr);
    virtual ~BottomTrack();
    virtual SceneObjectType type() const override;
    virtual bool eventFilter(QObject *watched, QEvent *event) override final;
    //QList<Epoch*> epochs() const;
    QMap<int,DatasetChannel> channels() const;
    DatasetChannel visibleChannel() const;
    void setDatasetPtr(Dataset* datasetPtr);
    void actionEvent(ActionEvent actionEvent);

public Q_SLOTS:
    virtual void setData(const QVector<QVector3D>& data, int primitiveType = GL_POINTS) override;
    virtual void clearData() override;
    void isEpochsChanged(int lEpoch, int rEpoch);
    void resetVertexSelection();
    void setVisibleChannel(int channelIndex);
    void setVisibleChannel(const DatasetChannel& channel);
    void selectEpoch(int epochIndex, int channelId);
    void surfaceUpdated();
    void surfaceStateChanged(bool state);

Q_SIGNALS:
    void epochHovered(int epochIndex);
    void epochPressed(int epochIndex);
    void epochErased(int epochIndex);
    void epochSelected(int epochIndex, int channelId);
    void epochListChanged();
    void visibleChannelChanged(int channelId);
    void visibleChannelChanged(DatasetChannel channel);

protected:
    friend class GraphicsScene3dView;

    virtual void mouseMoveEvent(Qt::MouseButtons buttons, qreal x, qreal y) override;
    virtual void mousePressEvent(Qt::MouseButtons buttons, qreal x, qreal y) override;
    virtual void mouseReleaseEvent(Qt::MouseButtons buttons, qreal x, qreal y) override;
    virtual void keyPressEvent(Qt::Key key) override;
    void updateRenderData(int lEpoch = 0, int rEpoch = 0);

private:
    QVector<QPair<int, int>> getSubarrays(const QVector<int>& sequenceVector); // TODO: to utils

    using EpochIndex = int;
    using VerticeIndex = int;
    QHash<VerticeIndex,EpochIndex> epochIndexMatchingMap_;
    DatasetChannel visibleChannel_;
    Dataset* datasetPtr_;
    QVector<QVector3D> renderData_;
};
