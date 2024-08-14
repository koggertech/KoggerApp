#include "usbl_view.h"
// #include <boundarydetector.h>
// #include <Triangle.h>
// #include <drawutils.h>

UsblView::UsblView(QObject* parent) :
    SceneObject(new UsblViewRenderImplementation, parent),
    datasetPtr_(nullptr)
{
}

UsblView::~UsblView()
{

}

SceneObject::SceneObjectType UsblView::type() const
{
    return SceneObjectType::UsblView;
}

void UsblView::setDatasetPtr(Dataset* datasetPtr)
{
    datasetPtr_ = datasetPtr;
}

void UsblView::setPointRadius(float radius)
{
    RENDER_IMPL(UsblView)->pointRadius_ = radius;

    Q_EMIT changed();
}

void UsblView::setTrackVisible(bool state)
{
    RENDER_IMPL(UsblView)->isTrackVisible_ = state;

    Q_EMIT changed();
}

void UsblView::setData(const QVector<QVector3D>& data, int primitiveType)
{
    SceneObject::setData(data, primitiveType);

    Q_EMIT changed();
}

void UsblView::clearData()
{
    SceneObject::clearData();
}

UsblView::UsblViewRenderImplementation::UsblViewRenderImplementation() :
    pointRadius_(5.0f),
    isTrackVisible_(true)
{

}

void UsblView::UsblViewRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp,
                                                    const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible) {
        return;
    }

    if (m_data.isEmpty()) {
        return;
    }

    auto shaderProgram = shaderProgramMap.value("static", nullptr);

    if (!shaderProgram) {
        qWarning() << "Shader program 'mosaic' not found!";
        return;
    }

    shaderProgram->bind();

    int posLoc     = shaderProgram->attributeLocation ("position");
    int matrixLoc  = shaderProgram->uniformLocation   ("matrix");
    int colorLoc   = shaderProgram->uniformLocation   ("color");
    int widthLoc   = shaderProgram->uniformLocation   ("width");
    int isPointLoc = shaderProgram->uniformLocation   ("isPoint");

    shaderProgram->setUniformValue(matrixLoc, mvp);
    shaderProgram->enableAttributeArray(posLoc);

    auto lineColor = QVector4D(m_color.redF(), m_color.greenF(), m_color.blueF(), 1.0f);
    auto subPointColor = QVector4D(0.9f, 0.9f, 0.9f, 1.0f);

    // point
    ctx->glEnable(GL_PROGRAM_POINT_SIZE);
    shaderProgram->setUniformValue(isPointLoc, true);
    QVector<QVector3D> point{ m_data.last() };
    // color point
    shaderProgram->setUniformValue(colorLoc, lineColor);
    shaderProgram->setUniformValue(widthLoc, pointRadius_);
    shaderProgram->setAttributeArray(posLoc, point.constData());
    ctx->glDrawArrays(GL_POINTS, 0, point.size());
    // gray point
    shaderProgram->setUniformValue(colorLoc, subPointColor);
    shaderProgram->setUniformValue(widthLoc, pointRadius_ * 1.2f);
    shaderProgram->setAttributeArray(posLoc, point.constData());
    ctx->glDrawArrays(GL_POINTS, 0, point.size());
    ctx->glDisable(GL_PROGRAM_POINT_SIZE);

    if (isTrackVisible_) {
        shaderProgram->setUniformValue(colorLoc, lineColor);
        shaderProgram->setUniformValue(isPointLoc, false);
        shaderProgram->setUniformValue(widthLoc, m_width);
        shaderProgram->setAttributeArray(posLoc, m_data.constData());
        ctx->glLineWidth(m_width);
        ctx->glDrawArrays(GL_LINE_STRIP, 0, m_data.size());
        ctx->glLineWidth(1.0f);
    }

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}

void UsblView::updateData()
{
    // some update

    emit changed();
}
