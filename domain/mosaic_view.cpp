#include "mosaic_view.h"
#include <boundarydetector.h>
#include <Triangle.h>
#include <drawutils.h>


MosaicView::MosaicView(QObject* parent) :
    SceneObject(new MosaicViewRenderImplementation, parent)
{

}

MosaicView::~MosaicView()
{

}

SceneObject::SceneObjectType MosaicView::type() const
{
    return SceneObjectType::MosaicView;
}

void MosaicView::setData(const QVector<QVector3D>& data, int primitiveType)
{
    SceneObject::setData(data, primitiveType);

    Q_EMIT changed();
}

void MosaicView::clearData()
{
    SceneObject::clearData();
}

void MosaicView::MosaicViewRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible)
        return;

    if (!shaderProgramMap.contains("height"))
        return;

    auto shaderProgram = shaderProgramMap["height"];

    if (!shaderProgram->bind()){
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

    shaderProgram->setAttributeArray(posLoc, m_data.constData());
    ctx->glDrawArrays(GL_QUADS, 0, m_data.size());

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}
