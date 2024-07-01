#include "boattrack.h"

#include <QtOpenGLExtensions/QOpenGLExtensions>
#include <QHash>
#include "graphicsscene3dview.h"


BoatTrack::BoatTrack(GraphicsScene3dView* view, QObject* parent)
    : SceneObject(new BoatTrackRenderImplementation,view,parent)
{

}

BoatTrack::~BoatTrack()
{

}

SceneObject::SceneObjectType BoatTrack::type() const
{
    return SceneObject::SceneObjectType::BoatTrack;
}

void BoatTrack::setData(const QVector<QVector3D> &data, int primitiveType)
{
    SceneObject::setData(data, primitiveType);
}

void BoatTrack::clearData()
{
    SceneObject::clearData();
}

//-----------------------RenderImplementation-----------------------------//
BoatTrack::BoatTrackRenderImplementation::BoatTrackRenderImplementation()
{

}

BoatTrack::BoatTrackRenderImplementation::~BoatTrackRenderImplementation()
{

}

void BoatTrack::BoatTrackRenderImplementation::render(QOpenGLFunctions *ctx,
                                                      const QMatrix4x4 &mvp,
                                                      const QMap<QString,
                                                      std::shared_ptr<QOpenGLShaderProgram> > &shaderProgramMap) const
{
    if(!m_isVisible)
        return;

    SceneObject::RenderImplementation::render(ctx, mvp, shaderProgramMap);
}

void BoatTrack::BoatTrackRenderImplementation::render(QOpenGLFunctions *ctx,
                                                      const QMatrix4x4 &model,
                                                      const QMatrix4x4 &view,
                                                      const QMatrix4x4 &projection,
                                                      const QMap<QString,std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if(!m_isVisible)
        return;    

    SceneObject::RenderImplementation::render(ctx,model,view,projection,shaderProgramMap);
}
