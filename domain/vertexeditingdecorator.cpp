#include "vertexeditingdecorator.h"
#include <draw_utils.h>

VertexEditingDecorator::VertexEditingDecorator(QObject *parent)
    : SceneObject(new VertexEditingDecoratorRenderImplementation, parent)
{}

VertexEditingDecorator::VertexEditingDecorator(std::weak_ptr<SceneObject> decoratedObject,
                                               int decoratedVertexIndex,
                                               QObject *parent)
    : SceneObject(new VertexEditingDecoratorRenderImplementation, parent)
    , m_decoratedObject(decoratedObject)
    , m_decoratedVertexIndex(decoratedVertexIndex)
{}

VertexEditingDecorator::~VertexEditingDecorator()
{}

bool VertexEditingDecorator::isValid() const
{
    return m_isValid;
}

void VertexEditingDecorator::setDecorated(std::weak_ptr<SceneObject> decoratedObject, int decoratedVertexIndex)
{
    m_decoratedObject = decoratedObject;
    m_decoratedVertexIndex = decoratedVertexIndex;

    if(m_decoratedObject.expired() ||
       decoratedVertexIndex >= decoratedObject.lock()->cdata().size() ||
       decoratedVertexIndex < 0)
    {
        m_isValid = false;
        return;
    }

    RENDER_IMPL(VertexEditingDecorator)->setData(
                {decoratedObject.lock()->cdata().at(decoratedVertexIndex)}
            );

    m_isValid = true;
}

void VertexEditingDecorator::moveVertex(QVector3D pos)
{
    Q_UNUSED(pos)
}

void VertexEditingDecorator::removeVertex(bool decorateNextVertex)
{
    if(!m_isValid)
        return;

    if(m_decoratedObject.expired() || m_decoratedVertexIndex < 0)
    {
        m_isValid = false;
        return;
    }

    m_decoratedObject.lock()->removeVertex(m_decoratedVertexIndex);

    if(m_decoratedVertexIndex >= m_decoratedObject.lock()->cdata().size() ||
       m_decoratedVertexIndex < 0)
    {
        m_isValid = false;
        return;
    }

    if(decorateNextVertex){
        RENDER_IMPL(VertexEditingDecorator)->setData({m_decoratedObject.lock()->cdata().at(m_decoratedVertexIndex)});
    }
}

void VertexEditingDecorator::clearData()
{
    m_isValid = false;
    RENDER_IMPL(VertexEditingDecorator)->clearData();
}

//-----------------------RenderImplementation-----------------------------//

VertexEditingDecorator::VertexEditingDecoratorRenderImplementation::VertexEditingDecoratorRenderImplementation()
{
    m_width = 20.0f;
    m_color = {255, 120, 120};
}

VertexEditingDecorator::VertexEditingDecoratorRenderImplementation::~VertexEditingDecoratorRenderImplementation()
{}

void VertexEditingDecorator::VertexEditingDecoratorRenderImplementation::render(QOpenGLFunctions *ctx,
                                                                                const QMatrix4x4 &mvp,
                                                                                const QMap<QString,
                                                                                std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if(!m_isVisible)
        return;

    if(!shaderProgramMap.contains("static"))
        return;

    auto shaderProgram = shaderProgramMap["static"];

    if (!shaderProgram->bind()){
        qCritical() << "Error binding shader program.";
        return;
    }
    QVector<QVector3D> testLine;
    if(!m_data.isEmpty()){
            testLine = {m_data.at(0),
                       {m_data.at(0).x(), m_data.at(0).y() + 5, m_data.at(0).z()}};
    }

    ctx->glEnable(34370);

    int posLoc    = shaderProgram->attributeLocation("position");
    int matrixLoc = shaderProgram->uniformLocation("matrix");
    int colorLoc  = shaderProgram->uniformLocation("color");
    int widthLoc  = shaderProgram->uniformLocation("width");

    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(m_color));
    shaderProgram->setUniformValue(matrixLoc, mvp);
    shaderProgram->setUniformValue(widthLoc, static_cast <float>(m_width));
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, m_data.constData());

    ctx->glDrawArrays(m_primitiveType, 0, m_data.size());

    shaderProgram->setAttributeArray(posLoc, testLine.constData());
    ctx->glDrawArrays(GL_LINES, 0, testLine.size());

    ctx->glDisable(34370);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}
