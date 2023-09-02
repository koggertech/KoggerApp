#include "graphicsscene3dview.h"

#include <QOpenGLFramebufferObject>

GraphicsScene3dView::GraphicsScene3dView()
: QQuickFramebufferObject()
{
    setAcceptedMouseButtons(Qt::AllButtons);
}

GraphicsScene3dView::~GraphicsScene3dView()
{}

QQuickFramebufferObject::Renderer *GraphicsScene3dView::createRenderer() const
{
    return new GraphicsScene3dView::GraphicsScene3dRenderer();
}

void GraphicsScene3dView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    if(m_scene)
        m_scene->setRect(newGeometry);

    QQuickFramebufferObject::geometryChanged(newGeometry, oldGeometry);
}

void GraphicsScene3dView::setScene(std::shared_ptr<GraphicsScene3d> scene)
{
    if(m_scene == scene)
        return;

    m_scene = scene;
    m_scene->setView(this);
    m_scene->setRect(boundingRect());

    update();
}

std::shared_ptr<GraphicsScene3d> GraphicsScene3dView::scene() const
{
    return m_scene;
}

void GraphicsScene3dView::mouseMoveTrigger(Qt::MouseButtons buttons, qreal x, qreal y)
{
    if(m_scene){
        if(buttons.testFlag(Qt::LeftButton)){
            qreal yaw = (x - m_lastMousePos.x()) * 0.002f;
            qreal pitch = (m_lastMousePos.y() - y) * 0.002f;

            m_scene->rotateCamera(pitch, yaw);
        }
    }

    m_lastMousePos = {x,y};
}

void GraphicsScene3dView::mousePressTrigger(Qt::MouseButtons buttons, qreal x, qreal y)
{
    Q_UNUSED(buttons)
    Q_UNUSED(x)
    Q_UNUSED(y)

    update();
}

void GraphicsScene3dView::mouseReleaseTrigger(Qt::MouseButtons buttons, qreal x, qreal y)
{
    Q_UNUSED(buttons)
    Q_UNUSED(x)
    Q_UNUSED(y)
}

void GraphicsScene3dView::mouseWheelTrigger(Qt::MouseButtons buttons, qreal x, qreal y, QPointF angleDelta)
{
    Q_UNUSED(buttons)
    Q_UNUSED(x)
    Q_UNUSED(y)

    if(!m_scene)
        return;

    m_scene->setCameraZoom((angleDelta.y() * 1.8f) * 0.05);
}

GraphicsScene3dView::GraphicsScene3dRenderer::GraphicsScene3dRenderer()
    :QQuickFramebufferObject::Renderer()
{
    _scene.initialize();
}

GraphicsScene3dView::GraphicsScene3dRenderer::~GraphicsScene3dRenderer()
{}

void GraphicsScene3dView::GraphicsScene3dRenderer::render()
{
    if(m_scene)
        m_scene->draw();

    update();
}

void GraphicsScene3dView::GraphicsScene3dRenderer::synchronize(QQuickFramebufferObject * fbo)
{
    auto view = qobject_cast<GraphicsScene3dView*>(fbo);

    if(!view)
        return;

    if(m_scene != view->scene())
        m_scene = view->scene();

    if(!m_scene)
        return;

    if(!m_scene->isInitialized())
        m_scene->initialize();

    m_scene->setRect(view->boundingRect());
}

QOpenGLFramebufferObject *GraphicsScene3dView::GraphicsScene3dRenderer::createFramebufferObject(const QSize &size)
{
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    return new QOpenGLFramebufferObject(size, format);
}
