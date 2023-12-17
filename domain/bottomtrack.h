#ifndef BOTTOMTRACK_H
#define BOTTOMTRACK_H

#include <memory>
#include <sceneobject.h>
#include <plotcash.h>

class GraphicsScene3dView;
class Surface;
class Epoch;
class BottomTrack : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(BottomTrack)

public:
    class BottomTrackRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        BottomTrackRenderImplementation();
        virtual ~BottomTrackRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx,
                            const QMatrix4x4& mvp,
                            const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;
    private:
        friend class BottomTrack;
        QVector<int> m_selectedVertexIndices;
        bool m_isDisplayingWithSurface = true;
    };

    explicit BottomTrack(GraphicsScene3dView* view = nullptr, QObject* parent = nullptr);
    virtual ~BottomTrack();
    virtual SceneObjectType type() const override;

public Q_SLOTS:
    virtual void setData(const QVector<QVector3D>& data, int primitiveType = GL_POINTS) override;
    virtual void clearData() override;
    void setEpochs(const QList<Epoch*>& epochList,const QMap<int,DatasetChannel>& channels);
    void resetVertexSelection();
    void setDisplayingWithSurface(bool displaying);


Q_SIGNALS:
    void epochHovered(int epochIndex);
    void epochPressed(int epochIndex);
    void epochErased(int epochIndex);
    void epochListChanged();

protected:
    friend class GraphicsScene3dView;

    virtual void mouseMoveEvent(Qt::MouseButtons buttons, qreal x, qreal y) override;
    virtual void mousePressEvent(Qt::MouseButtons buttons, qreal x, qreal y) override;
    virtual void mouseReleaseEvent(Qt::MouseButtons buttons, qreal x, qreal y) override;
    virtual void keyPressEvent(Qt::Key key) override;
    void updateRenderData();

private:
    using EpochIndex = int;
    using VerticeIndex = int;

    QList<Epoch*> m_epochList;
    QMap<int, DatasetChannel> m_channels;
    QHash<VerticeIndex,EpochIndex> m_epochIndexMatchingMap;
    LLARef m_llaRef;
    DatasetChannel m_visibleChannel;
};

#endif // BOTTOMTRACK_H
