#include "graphicsscene3d.h"

#include <QtMath>

GraphicsScene3d::GraphicsScene3d()
{
    m_shaderProgramMap["height"] = std::make_shared<QOpenGLShaderProgram>();
    m_shaderProgramMap["static"] = std::make_shared<QOpenGLShaderProgram>();
}

GraphicsScene3d::~GraphicsScene3d()
{

}

void GraphicsScene3d::addGraphicsObject(std::shared_ptr<SceneGraphicsObject> object)
{
    if(m_objectList.contains(object))
        return;

    object->setScene(std::shared_ptr <GraphicsScene3d>(this));

    m_objectList.append(object);
}

void GraphicsScene3d::removeGraphicsObject(std::shared_ptr<SceneGraphicsObject> object)
{
    if(!m_objectList.contains(object))
        return;

    m_objectList.removeOne(object);
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

    //view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);


    qreal sphereRadius = -500;

    QVector3D cameraTarget(0.0f, 0.0f, 0.0f);

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

    //QVector3D cameraPosition(0.0f, 0.0f, 163.0f);

    //QVector3D cameraDirection = (cameraPosition-cameraTarget).normalized();
    //QVector3D cameraRight = QVector3D::crossProduct({0.0f, 1.0f, 0.0f}, cameraDirection);
    //QVector3D cameraUp    = QVector3D::crossProduct(cameraDirection, //);

    QMatrix4x4 model, view, projection;

    view.lookAt(cameraPosition + cameraTarget, cameraTarget, cameraUp.normalized());
    projection.perspective(m_fov, m_rect.width()/m_rect.height(), 1.0f, 5000.0f);

    m_projection = std::move(projection);
    m_model      = std::move(model);
    m_view       = std::move(view);

    drawObjects();

    glDisable(GL_DEPTH_TEST);
}

void GraphicsScene3d::drawObjects()
{
    //for(const auto& object : m_objectList)
    //    object->draw(this, m_projection * m_view * m_model, m_shaderProgramMap);

    if(!m_shaderProgramMap.contains("static"))
        return;

    auto shaderProgram = m_shaderProgramMap["static"];

    if (!shaderProgram->bind()){
        qCritical() << "Error binding shader program.";
        return;
    }

    int posLoc    = shaderProgram->attributeLocation("position");
    int matrixLoc = shaderProgram->uniformLocation("matrix");
    int colorLoc  = shaderProgram->uniformLocation("color");

    QVector4D color(1.0f, 1.0f, 0.0f, 0.0f);

    QVector <QVector3D> data{
        {0.0f, 2.0f, 12.0f},
        {10.0f, 2.0f, 12.0f},
        {18.0f, 12.0f, 22.0f},
        {15.0f, 62.0f, 12.0f},
    };

    //qDebug() << "p: " << m_projection <<", v: " << m_view << ", m: " << m_model;

    shaderProgram->setUniformValue(colorLoc, color);
    shaderProgram->setUniformValue(matrixLoc, m_projection * m_view * m_model);
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, data.constData());

    glDrawArrays(GL_POLYGON, 0, data.size());

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();

    qDebug() << "Rendering...";
}
