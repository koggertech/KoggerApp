#include "boat_track.h"

#include <QtOpenGLExtensions/QOpenGLExtensions>
#include <QHash>
#include "scene3d_view.h"
#include "epoch_event.h"


BoatTrack::BoatTrack(GraphicsScene3dView* view, QObject* parent) :
    SceneObject(new BoatTrackRenderImplementation, view, parent),
    datasetPtr_(nullptr)
{
    setPrimitiveType(GL_LINE_STRIP);
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
    if (m_view->m_mode == GraphicsScene3dView::ActiveMode::Idle) {
        return false;
    }
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

void BoatTrack::onPositionAdded(uint64_t indx)
{
    if (!datasetPtr_) {
        return;
    }

    const int toIndx = indx;
    const int fromIndx = 0; //lastEpoch_;
    if (fromIndx > toIndx) {
        return;
    }

    QVector<QVector3D> prepData(toIndx, QVector3D());
    selectedIndices_.clear();
    uint64_t validPosCounter = 0;

    for (int i = fromIndx; i < toIndx; ++i) {
        if (auto* ep = datasetPtr_->fromIndex(i); ep) {
            if (auto posNed = ep->getPositionGNSS().ned; posNed.isCoordinatesValid()) {
                prepData[i] = QVector3D(posNed.n, posNed.e, 0);
                selectedIndices_.insert(validPosCounter++, i);
            }
        }
    }

    SceneObject::setData(prepData, GL_LINE_STRIP);
}

void BoatTrack::clearData()
{
    auto r = RENDER_IMPL(BoatTrack);
    r->boatTrackVertice_ = QVector3D();
    r->bottomTrackVertice_ = QVector3D();

    selectedIndices_.clear();

    SceneObject::clearData();
}

void BoatTrack::selectEpoch(int epochIndex)
{
    if (epochIndex < 0 || epochIndex >= datasetPtr_->size())
        return;

    if (auto* epoch = datasetPtr_->fromIndex(epochIndex); epoch) {
        NED epNed = epoch->getPositionGNSS().ned;

        if (epNed.isCoordinatesValid()) {
            auto* r = RENDER_IMPL(BoatTrack);
            r->boatTrackVertice_ = QVector3D(epNed.n, epNed.e, 0.0f);

            // channel select logic from bottomTrack
            bool beenBottomSelected{ false };

            if (datasetPtr_) {
                if (auto datasetChannels = datasetPtr_->channelsList(); !datasetChannels.isEmpty()) {
                    if (float distance = -1.f * static_cast<float>(epoch->distProccesing(datasetChannels.first().channelId_)); isfinite(distance)) {
                        r->bottomTrackVertice_ = QVector3D(epNed.n, epNed.e, distance);
                        beenBottomSelected = true;
                    }
                }
            }
            if (!beenBottomSelected) {
                r->bottomTrackVertice_ = QVector3D();
            }
        }
        else {
            //qDebug() << "invalid pos on boat track" << epochIndex;
        }
    }

    Q_EMIT changed();
}

void BoatTrack::setBottomTrackVisibleState(bool state)
{
    auto* r = RENDER_IMPL(BoatTrack);
    r->bottomTrackVisibleState_ = state;
}

void BoatTrack::clearSelectedEpoch()
{
    auto* r = RENDER_IMPL(BoatTrack);
    r->boatTrackVertice_ = QVector3D();
    r->bottomTrackVertice_ = QVector3D();
}

void BoatTrack::mousePressEvent(Qt::MouseButtons buttons, qreal x, qreal y)
{
    Q_UNUSED(buttons)
    Q_UNUSED(x)
    Q_UNUSED(y)

    if (!m_view)
        return;

    if (m_view->m_mode == GraphicsScene3dView::BottomTrackVertexSelectionMode) {
        if (buttons.testFlag(Qt::LeftButton)) {
            if (m_view->bottomTrack()->data().empty()) {
                auto hits = m_view->m_ray.hitObject(shared_from_this(), Ray::HittingMode::Vertex);
                if (!hits.isEmpty()) {
                    auto indice = hits.first().indices().first;
                    if (selectedIndices_.size() > (indice + 1)) {
                        auto epochIndx = selectedIndices_[indice];
                        if (auto* epoch = datasetPtr_->fromIndex(epochIndx); epoch) {
                            NED epNed = epoch->getPositionGNSS().ned;

                            QVector3D pos(epNed.n, epNed.e, 0.0f);
                            RENDER_IMPL(BoatTrack)->boatTrackVertice_ = pos;

                            if (auto datasetChannels = datasetPtr_->channelsList(); !datasetChannels.isEmpty()) {
                                auto epochEvent = new EpochEvent(EpochSelected3d, epoch, epochIndx, datasetChannels.first());
                                QCoreApplication::postEvent(this, epochEvent);
                            }
                        }
                    }
                }
            }
        }
    }
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
                                                      const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible)
        return;

    SceneObject::RenderImplementation::render(ctx, model, view, projection, shaderProgramMap);

    //------------->Drawing selected vertice<<---------------//
    if (boatTrackVertice_.isNull()) {
        return;
    }

    auto shaderProgram = shaderProgramMap["static"].get();
    shaderProgram->bind();

    auto colorLoc  = shaderProgram->uniformLocation("color");
    auto matrixLoc = shaderProgram->uniformLocation("matrix");
    auto posLoc    = shaderProgram->attributeLocation("position");
    int widthLoc   = shaderProgram->uniformLocation("width");

    //QVector4D vertexColor(0.03f, 0.69f, 0.98f, 1.0f);
    QVector4D vertexColor(0.91f, 0.25f, 0.2f, 1.0f);

    shaderProgram->setUniformValue(colorLoc,vertexColor);
    shaderProgram->setUniformValue(matrixLoc, projection * view * model);
    shaderProgram->setUniformValue(widthLoc, 12.0f);
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, &boatTrackVertice_);

    ctx->glEnable(34370);
    ctx->glDrawArrays(GL_POINTS, 0, 1);
    ctx->glDisable(34370);

    shaderProgram->disableAttributeArray(posLoc);

    //------------->Drawing line boatTrack -> bottomTrack<<---------------//
    if (bottomTrackVertice_.isNull() || !bottomTrackVisibleState_) {
        return;
    }

    QVector4D lineColor(0.91f, 0.25f, 0.2f, 1.0f);
    shaderProgram->setUniformValue(colorLoc, lineColor);

    QVector<QVector3D> vertices{ boatTrackVertice_, bottomTrackVertice_ };

    ctx->glLineWidth(2);
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, vertices.constData(), sizeof(QVector3D));

    ctx->glDrawArrays(GL_LINES, 0, 2);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}
