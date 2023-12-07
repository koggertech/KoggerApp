#include "polygonobject.h"

#include <QModelIndex>

#include <drawutils.h>
#include <pointobject.h>

PolygonObject::PolygonObject(QObject *parent)
    : PointGroup(new PolygonObjectRenderImplementation, parent)
{}

PolygonObject::~PolygonObject()
{}

SceneObject::SceneObjectType PolygonObject::type() const
{
    return SceneObject::SceneObjectType::Polygon;
}

//-----------------------RenderImplementation-----------------------------//
PolygonObject::PolygonObjectRenderImplementation::PolygonObjectRenderImplementation()
{}

PolygonObject::PolygonObjectRenderImplementation::~PolygonObjectRenderImplementation()
{}

void PolygonObject::PolygonObjectRenderImplementation::render(QOpenGLFunctions *ctx,
                                                              const QMatrix4x4 &mvp,
                                                              const QMap<QString, std::shared_ptr<QOpenGLShaderProgram> > &shaderProgramMap) const
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

    int posLoc    = shaderProgram->attributeLocation("position");
    int colorLoc  = shaderProgram->uniformLocation("color");
    int matrixLoc = shaderProgram->uniformLocation("matrix");

    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(m_color));
    shaderProgram->setUniformValue(matrixLoc, mvp);
    shaderProgram->enableAttributeArray(posLoc);

    QVector <QVector3D> data;

    for(const auto& renderImpl : m_pointRenderImplList)
        data.append(renderImpl.cdata().at(0));

    shaderProgram->setAttributeArray(posLoc, data.constData());

    ctx->glDrawArrays(GL_POLYGON, 0, data.count());

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}
