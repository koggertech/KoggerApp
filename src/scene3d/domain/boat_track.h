#pragma once

#include "scene_object.h"
#include "dataset.h"


class GraphicsScene3dView;
class BoatTrack : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(BoatTrack)

public:
    class BoatTrackRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        BoatTrackRenderImplementation();
        virtual ~BoatTrackRenderImplementation();

        virtual void render(QOpenGLFunctions* ctx,
                            const QMatrix4x4& mvp,
                            const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override final;

        virtual void render(QOpenGLFunctions* ctx,
                            const QMatrix4x4& model,
                            const QMatrix4x4& view,
                            const QMatrix4x4& projection,
                            const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override final;

    private:
        friend class BoatTrack;
        QVector3D boatTrackVertice_;
        QVector3D bottomTrackVertice_;
        bool bottomTrackVisibleState_ = true;
    };

    explicit BoatTrack(GraphicsScene3dView* view = nullptr, QObject* parent = nullptr);
    virtual ~BoatTrack();
    virtual SceneObjectType type() const override final;
    virtual bool eventFilter(QObject *watched, QEvent *event) override final;
    void setDatasetPtr(Dataset* datasetPtr);
    void onPositionAdded(uint64_t indx);

public Q_SLOTS:
    virtual void clearData() override final;
    void selectEpoch(int epochIndex);
    void setBottomTrackVisibleState(bool state);
    void clearSelectedEpoch();
    virtual void mousePressEvent(Qt::MouseButtons buttons, qreal x, qreal y) override final;

Q_SIGNALS:

protected:
    friend class GraphicsScene3dView;
    friend class BottomTrack;

private:
    Dataset* datasetPtr_;
    int lastIndx_;
};
