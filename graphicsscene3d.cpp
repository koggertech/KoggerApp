#include "graphicsscene3d.h"

#include <QtMath>

#include <bottomtrack.h>
#include <surface.h>
#include <pointgroup.h>
#include <polygongroup.h>
#include <scenegraphicsobject.h>

static const qreal sphereRadius = -500.0f;

GraphicsScene3d::GraphicsScene3d(QObject* parent)
    : QObject(parent)
    , m_bottomTrack(std::make_shared<BottomTrack>())
    , m_surface(std::make_shared<Surface>())
    , m_pointGroup(std::make_shared<PointGroup>())
    , m_polygonGroup(std::make_shared<PolygonGroup>())
{
    m_shaderProgramMap["height"] = std::make_shared<QOpenGLShaderProgram>();
    m_shaderProgramMap["static"] = std::make_shared<QOpenGLShaderProgram>();
}

GraphicsScene3d::~GraphicsScene3d()
{}

void GraphicsScene3d::setBottomTrack(std::shared_ptr<BottomTrack> bottomTrack)
{
    m_bottomTrack = bottomTrack;
}

void GraphicsScene3d::setSurface(std::shared_ptr<Surface> surface)
{
    m_surface = surface;
}

void GraphicsScene3d::addGraphicsObject(std::shared_ptr<SceneGraphicsObject> object)
{
    if(m_objectList.contains(object))
        return;

    object->setScene(this);

    m_objectList.append(object);

    Q_EMIT objectsCountChanged(m_objectList);
}

void GraphicsScene3d::removeGraphicsObject(std::shared_ptr<SceneGraphicsObject> object)
{
    if(!m_objectList.contains(object))
        return;

    m_objectList.removeOne(object);

    Q_EMIT objectsCountChanged(m_objectList);
}

GraphicsScene3dView *GraphicsScene3d::view() const
{
    return mp_view;
}

bool GraphicsScene3d::isInitialized() const
{
    return m_isInitialized;
}

qreal GraphicsScene3d::fov() const
{
    return m_fov;
}

void GraphicsScene3d::rotateCamera(qreal pitchOffset, qreal yawOffset)
{
    m_pitch += pitchOffset;
    m_yaw   += yawOffset;

    if(m_pitch > 89.0f)
      m_pitch =  89.0f;
    if(m_pitch < -89.0f)
      m_pitch = -89.0f;
}

void GraphicsScene3d::setCameraZoom(qreal angle)
{
    if(m_fov >= 1.0f && m_fov <= 45.0f)
      m_fov -= angle;
    if(m_fov <= 1.0f)
      m_fov = 1.0f;
    if(m_fov >= 45.0f)
      m_fov = 45.0f;
}

void GraphicsScene3d::setView(GraphicsScene3dView *view)
{
    if(mp_view == view)
        return;

    mp_view = view;
}

void GraphicsScene3d::setRect(const QRectF &rect)
{
    if(m_rect != rect)
        m_rect = rect;
}

void GraphicsScene3d::setWidth(qreal width)
{
    m_rect.setWidth(width);
}

void GraphicsScene3d::setHeight(qreal height)
{
    m_rect.setHeight(height);
}

void GraphicsScene3d::setDragOffset(QVector3D offset)
{
    m_dragOffset = offset;
}

void GraphicsScene3d::setRotationAngle(QVector2D angle)
{
    m_rotationAngle = angle;
}

QRectF GraphicsScene3d::rect() const
{
    return m_rect;
}

qreal GraphicsScene3d::width() const
{
    return m_rect.width();
}

qreal GraphicsScene3d::height() const
{
    return m_rect.height();
}

void GraphicsScene3d::initialize()
{
    initializeOpenGLFunctions();

    m_isInitialized = true;

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.3f, 0.3f, 0.3f, 0.0f);

    // ---------
    bool success = m_shaderProgramMap["static"]->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/base.vsh");

    if (!success) qCritical() << "Error adding vertex shader from source file.";

    success = m_shaderProgramMap["static"]->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/staticcolor.fsh");

    if (!success) qCritical() << "Error adding fragment shader from source file.";

    success = m_shaderProgramMap["static"]->link();

    if (!success) qCritical() << "Error linking shaders in shader program.";

    // --------
    success = m_shaderProgramMap["height"]->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/base.vsh");

    if (!success) qCritical() << "Error adding vertex shader from source file.";

    success = m_shaderProgramMap["height"]->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/heightcolor.fsh");

    if (!success) qCritical() << "Error adding fragment shader from source file.";

    success = m_shaderProgramMap["height"]->link();

    if (!success) qCritical() << "Error linking shaders in shader program.";
}

void GraphicsScene3d::draw()
{
    glDepthMask(true);

    glClearColor(0.3f, 0.3f, 0.3f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    glFrontFace(GL_CW);
    glEnable(GL_DEPTH_TEST);

    m_center[0] += (m_dragOffset[1]*cosf(-m_yaw)*cosf(m_rotationAngle.y()) - m_dragOffset[0]*sinf(-m_yaw));
    m_center[1] += (m_dragOffset[1]*sinf(-m_yaw)*cosf(m_rotationAngle.y()) + m_dragOffset[0]*cosf(-m_yaw));
    m_center[2] += -m_dragOffset[1]*sinf(m_pitch)*sinf(m_pitch);

    QVector3D cameraPosition(
                        -sinf(m_pitch)*cosf(-m_yaw)*sphereRadius,
                        -cosf(m_pitch)*sphereRadius,
                        -sinf(m_pitch)*sinf(-m_yaw)*sphereRadius
                    );

    QVector3D cameraUp(
                    cosf(m_pitch)*cosf(-m_yaw),
                    -sinf(m_pitch),
                    cosf(m_pitch)*sinf(-m_yaw)
                );

    auto cameraTarget = m_center;

    QMatrix4x4 model, view, projection;

    view.lookAt(cameraPosition + cameraTarget, cameraTarget, cameraUp.normalized());
    projection.perspective(m_fov, m_rect.width()/m_rect.height(), 1.0f, 5000.0f);

    m_projection = std::move(projection);
    m_model      = std::move(model);
    m_view       = std::move(view);

    drawObjects();

    glDisable(GL_DEPTH_TEST);
}

std::shared_ptr<BottomTrack> GraphicsScene3d::bottomTrack() const
{
    return m_bottomTrack;
}

std::shared_ptr<Surface> GraphicsScene3d::surface() const
{
    return m_surface;
}

std::shared_ptr<PointGroup> GraphicsScene3d::pointGroup() const
{
    return m_pointGroup;
}

std::shared_ptr<PolygonGroup> GraphicsScene3d::polygonGroup() const
{
    return m_polygonGroup;
}

QList<std::shared_ptr<SceneGraphicsObject> > GraphicsScene3d::objects() const
{
    return m_objectList;
}

void GraphicsScene3d::drawObjects()
{
    m_bottomTrack->draw(this, m_projection * m_view * m_model, m_shaderProgramMap);
    m_surface->draw(this, m_projection * m_view * m_model, m_shaderProgramMap);
    m_pointGroup->draw(this, m_projection * m_view * m_model, m_shaderProgramMap);
    m_polygonGroup->draw(this, m_projection * m_view * m_model, m_shaderProgramMap);
}
