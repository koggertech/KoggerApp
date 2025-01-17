#include "contacts.h"

#include <draw_utils.h>


Contacts::Contacts(QObject *parent) :
    SceneObject(new ContactsRenderImplementation, parent)
{

}

Contacts::~Contacts()
{

}

void Contacts::clear()
{

    auto renderImpl = RENDER_IMPL(Contacts);

    //

    renderImpl->createBounds();


    Q_EMIT changed();
    Q_EMIT boundsChanged();
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// ContactsRenderImplementation
Contacts::ContactsRenderImplementation::ContactsRenderImplementation()
{

}

void Contacts::ContactsRenderImplementation::render(QOpenGLFunctions *ctx,
                                                    const QMatrix4x4 &model,
                                                    const QMatrix4x4 &view,
                                                    const QMatrix4x4 &projection,
                                                    const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    auto shaderProgram = shaderProgramMap.value("static", nullptr);

    if (!shaderProgram) {
        qWarning() << "Shader program 'static' not found!";
        return;
    }

    shaderProgram->bind();

    int posLoc     = shaderProgram->attributeLocation ("position");
    int matrixLoc  = shaderProgram->uniformLocation   ("matrix");
    int colorLoc   = shaderProgram->uniformLocation   ("color");
    int widthLoc   = shaderProgram->uniformLocation   ("width");
    int isTriangleLoc = shaderProgram->uniformLocation   ("isTriangle");

    shaderProgram->setUniformValue(matrixLoc, projection * view * model);
    shaderProgram->enableAttributeArray(posLoc);

    // test point
    ctx->glEnable(34370);

    shaderProgram->setUniformValue(isTriangleLoc, true);
    shaderProgram->setUniformValue(colorLoc, QVector4D(1.0f, 0.0f, 0.0f, 1.0f));
    QVector<QVector3D> point{ {0.0f,0.0f, 0.0f} };
    shaderProgram->setUniformValue(widthLoc, 35.f);
    shaderProgram->setAttributeArray(posLoc, point.constData());
    ctx->glDrawArrays(GL_POINTS, 0, point.size());

    ctx->glDisable(34370);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->setUniformValue(isTriangleLoc, false);
    shaderProgram->release();

}
