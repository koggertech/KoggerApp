#include "boattrack.h"

#include <QtOpenGLExtensions/QOpenGLExtensions>
#include <QHash>
#include "graphicsscene3dview.h"
#include <epochevent.h>


BoatTrack::BoatTrack(GraphicsScene3dView* view, QObject* parent) :
    SceneObject(new BoatTrackRenderImplementation, view, parent),
    datasetPtr_(nullptr)
{

}

BoatTrack::~BoatTrack()
{

}

SceneObject::SceneObjectType BoatTrack::type() const
{
    return SceneObject::SceneObjectType::BoatTrack;
}

bool BoatTrack::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);

    if (event->type() == EpochSelected2d) {
        auto* epochEvent = static_cast<EpochEvent*>(event);
        clearSelectedEpoch();
        m_view->m_mode = GraphicsScene3dView::ActiveMode::BottomTrackVertexSelectionMode;
        selectEpoch(epochEvent->epochIndex());
        m_view->update();
    }
    return false;
}

void BoatTrack::setDatasetPtr(Dataset *datasetPtr)
{
    datasetPtr_ = datasetPtr;
}

void BoatTrack::setData(const QVector<QVector3D> &data, int primitiveType)
{
    SceneObject::setData(data, primitiveType);
}

void BoatTrack::clearData()
{
    SceneObject::clearData();
}

void BoatTrack::selectEpoch(int epochIndex)
{
    if (epochIndex < 0 || epochIndex >= datasetPtr_->size())
        return;

    if (auto* epoch = datasetPtr_->fromIndex(epochIndex); epoch) {
        if (auto ned = epoch->getPositionGNSS().ned; ned.isCoordinatesValid()) {
            auto r = RENDER_IMPL(BoatTrack);
            r->selectedVertice_ = QVector3D(ned.n, ned.e, 0.0f);
        }
    }

    Q_EMIT changed();
}

void BoatTrack::clearSelectedEpoch()
{
    auto r = RENDER_IMPL(BoatTrack);
    r->selectedVertice_ = QVector3D();
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

    SceneObject::RenderImplementation::render(ctx, model, view, projection, shaderProgramMap);

    //------------->Drawing selected vertice<<---------------//
    if (selectedVertice_.isNull()) {
        return;
    }

    auto shaderProgram = shaderProgramMap["static"].get();
    shaderProgram->bind();

    auto colorLoc  = shaderProgram->uniformLocation("color");
    auto matrixLoc = shaderProgram->uniformLocation("matrix");
    auto posLoc    = shaderProgram->attributeLocation("position");
    int widthLoc   = shaderProgram->uniformLocation("width");

    QVector4D vertexColor(0.0f, 0.3f, 1.0f, 1.0f);

    shaderProgram->setUniformValue(colorLoc,vertexColor);
    shaderProgram->setUniformValue(matrixLoc, projection * view * model);
    shaderProgram->setUniformValue(widthLoc, 10.0f);
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, &selectedVertice_);

#ifndef Q_OS_ANDROID
    ctx->glEnable(GL_PROGRAM_POINT_SIZE);
#else
    ctx->glEnable(34370);
#endif

    ctx->glLineWidth(4.0);
    ctx->glDrawArrays(GL_POINTS, 0, 1);
    ctx->glLineWidth(1.0);

#ifndef Q_OS_ANDROID
    ctx->glDisable(GL_PROGRAM_POINT_SIZE);
#else
    ctx->glDisable(34370);
#endif

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();

}
