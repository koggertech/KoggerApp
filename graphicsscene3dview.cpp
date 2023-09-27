#include "graphicsscene3dview.h"

#include <memory.h>

#include <QOpenGLFramebufferObject>
#include <QVector3D>
#include <surface.h>

GraphicsScene3dView::GraphicsScene3dView()
: QQuickFramebufferObject()
, m_rayCaster(std::make_shared<RayCaster>())
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

    auto old = m_scene;

    m_scene = scene;
    m_scene->setView(this);
    m_scene->setRect(boundingRect());

    m_rayCaster->addObject(
                    std::static_pointer_cast<SceneObject>(scene->surface())
                );

    Q_EMIT sceneChanged(old, m_scene);

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

            QVector2D lastMouse(m_lastMousePos.x(), m_lastMousePos.y());
            QVector2D mouse(x,y);

            m_scene->setRotationAngle((lastMouse - mouse)*0.002);
            m_scene->rotateCamera(pitch, yaw);
        }

        if(buttons.testFlag(Qt::MiddleButton)){
            //auto dragOffset = QVector3D(-(m_lastMousePos.x() - x), (m_lastMousePos.y() - y), 0)*(m_scene->fov()*0.02);

            //m_scene->setDragOffset(dragOffset);
        }
    }

    m_lastMousePos = {x,y};

    auto origin = QVector3D(m_lastMousePos.x(), m_lastMousePos.y(), -1.0f)
            .unproject(m_scene->model() * m_scene->view(),
                       m_scene->projection(),
                       m_scene->rect().toRect());

    auto end = QVector3D(m_lastMousePos.x(), m_lastMousePos.y(), 1.0f)
            .unproject(m_scene->model() * m_scene->view(),
                       m_scene->projection(),
                       m_scene->rect().toRect());

    auto direction = (end - origin).normalized();

    m_scene->lock();
    m_rayCaster->trigger(origin, direction);

    QList <std::shared_ptr <SceneGraphicsObject>> pickedObjects;

    for(const auto& hit : m_rayCaster->hits()){
        auto object = std::make_shared<SceneGraphicsObject>();

        object->setPrimitiveType(hit.sourcePrimitive().second);
        object->setData(hit.sourcePrimitive().first);

        pickedObjects.append(object);
    }

    m_scene->setGraphicsObjects(pickedObjects);
    m_scene->unlock();
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
{}

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
