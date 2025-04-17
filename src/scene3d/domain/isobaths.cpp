#include "isobaths.h"


Isobaths::Isobaths(QObject* parent) :
    SceneObject(new IsobathsRenderImplementation, parent)
{}

Isobaths::~Isobaths()
{}

void Isobaths::setProcessingTask(const IsobathsProcessorTask& task)
{
    processingTask_ = task;
}

SceneObject::SceneObjectType Isobaths::type() const
{
    return SceneObjectType::Isobaths;
}

IsobathsProcessorTask Isobaths::processingTask() const
{
    return processingTask_;
}

void Isobaths::setData(const QVector<QVector3D>& data, int primitiveType)
{
    SceneObject::setData(data, primitiveType);

    Q_EMIT changed();
}

void Isobaths::clearData()
{
    SceneObject::clearData();

    Q_EMIT changed();
}

void Isobaths::IsobathsRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible) {
        return;
    }

    if (!shaderProgramMap.contains("height")) {
        return;
    }

    auto shaderProgram = shaderProgramMap["height"];

    if (!shaderProgram->bind()) {
        qCritical() << "Error binding shader program.";
        return;
    }

    int posLoc    = shaderProgram->attributeLocation("position");
    int maxZLoc   = shaderProgram->uniformLocation("max_z");
    int minZLoc   = shaderProgram->uniformLocation("min_z");
    int matrixLoc = shaderProgram->uniformLocation("matrix");

    shaderProgram->setUniformValue(maxZLoc, m_bounds.maximumZ());
    shaderProgram->setUniformValue(minZLoc, m_bounds.minimumZ());
    shaderProgram->setUniformValue(matrixLoc, mvp);
    shaderProgram->enableAttributeArray(posLoc);

#if defined (Q_OS_ANDROID) || defined(LINUX_ES)
    if (primitiveType() == GL_TRIANGLES) {
        shaderProgram->setAttributeArray(posLoc, m_data.constData());
        ctx->glDrawArrays(m_primitiveType, 0, m_data.size());
    }
    // else if (primitiveType() == GL_QUADS) {
    //     shaderProgram->setAttributeArray(posLoc, quadSurfaceVertices_.constData());
    //     ctx->glDrawArrays(GL_TRIANGLES, 0, quadSurfaceVertices_.size());
    // }
#else
    shaderProgram->setAttributeArray(posLoc, m_data.constData());
    ctx->glDrawArrays(m_primitiveType, 0, m_data.size());
#endif

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}
