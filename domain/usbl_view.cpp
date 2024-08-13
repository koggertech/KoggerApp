#include "usbl_view.h"
// #include <boundarydetector.h>
// #include <Triangle.h>
// #include <drawutils.h>

UsblView::UsblView(QObject* parent) :
    SceneObject(new UsblViewRenderImplementation, parent)
{
}

UsblView::~UsblView()
{

}

SceneObject::SceneObjectType UsblView::type() const
{
    return SceneObjectType::UsblView;
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

void UsblView::UsblViewRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible)
        return;

    auto shaderProgram = shaderProgramMap.value("static", nullptr);

    if (!shaderProgram) {
        qWarning() << "Shader program 'mosaic' not found!";
        return;
    }

    shaderProgram->bind();

    shaderProgram->setUniformValue("mvp", mvp);
    int posLoc = shaderProgram->attributeLocation("position");

    shaderProgram->enableAttributeArray(posLoc);

    shaderProgram->setAttributeArray(posLoc, m_data.constData());

    //ctx->glDrawElements(GL_TRIANGLES, indices_.size(), GL_UNSIGNED_INT, indices_.constData());

    shaderProgram->disableAttributeArray(posLoc);

    shaderProgram->release();
}

void UsblView::updateData()
{
    // some update

    emit changed();
}
