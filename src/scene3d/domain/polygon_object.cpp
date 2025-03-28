#include "polygon_object.h"

#include <QModelIndex>

#include "draw_utils.h"
#include "point_object.h"

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

    for (int i = 1; i < data.size() - 1; ++i) {
        QVector<QVector3D> triangle;
        triangle.append(data[0]);
        triangle.append(data[i]);
        triangle.append(data[i + 1]);

        shaderProgram->setAttributeArray(posLoc, triangle.constData());
        ctx->glDrawArrays(GL_TRIANGLES, 0, triangle.size());
    }

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}
