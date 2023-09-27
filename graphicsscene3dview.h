#ifndef GRAPHICSSCENE3DVIEW_H
#define GRAPHICSSCENE3DVIEW_H

#include <QQuickFramebufferObject>

#include <graphicsscene3d.h>
#include <raycaster.h>

class GraphicsScene3dView : public QQuickFramebufferObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(GraphicsScene3dView)

public:

    class GraphicsScene3dRenderer : public QQuickFramebufferObject::Renderer
    {
    public:
        GraphicsScene3dRenderer();
        virtual ~GraphicsScene3dRenderer();

    protected:
        virtual void render() override;
        virtual void synchronize(QQuickFramebufferObject * fbo) override;
        virtual QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override;

    protected:
        std::shared_ptr <GraphicsScene3d> m_scene;
    };

    /**
     * @brief Constructor
     */
    GraphicsScene3dView();

    /**
     * @brief Destructor
     */
    virtual ~GraphicsScene3dView();

    /**
     * @brief Creates renderer
     * @return renderer
     */
    Renderer *createRenderer() const override;

    /**
     * @brief Sets scene for rendering
     * @param[in] scene pointer to scene for rendering
     */
    void setScene(std::shared_ptr <GraphicsScene3d> scene);

    /**
     * @brief returns pointer to rendering scene
     * @return[in] pointer to rendering scene
     */
    std::shared_ptr <GraphicsScene3d> scene() const;

    Q_INVOKABLE void mouseMoveTrigger(Qt::MouseButtons buttons, qreal x, qreal y);

    Q_INVOKABLE void mousePressTrigger(Qt::MouseButtons buttons, qreal x, qreal y);

    Q_INVOKABLE void mouseReleaseTrigger(Qt::MouseButtons buttons, qreal x, qreal y);

    Q_INVOKABLE void mouseWheelTrigger(Qt::MouseButtons buttons, qreal x, qreal y, QPointF angleDelta);

Q_SIGNALS:
    void sceneChanged(std::shared_ptr<GraphicsScene3d> oldScene,
                      std::shared_ptr<GraphicsScene3d> newScene);

protected:
    /**
     * @brief Overrided geometry changed method
     * @param newGeometry new geometry rect instanse
     * @param oldGeometry old geometry rect instanse
     */
    virtual void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    friend class GraphicsScene3dView;
    std::shared_ptr <GraphicsScene3d> m_scene;
    QPointF m_lastMousePos = {0.0f, 0.0f};
    std::shared_ptr <RayCaster> m_rayCaster;
};

#endif // GRAPHICSSCENE3DVIEW_H
