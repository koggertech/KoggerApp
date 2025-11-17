#include "bottom_track.h"
#include "scene3d_view.h"
#include "epoch_event.h"
#include "boat_track.h"
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <QHash>

BottomTrack::BottomTrack(GraphicsScene3dView* view, QObject* parent) :
    SceneObject(new BottomTrackRenderImplementation, view, parent),
    datasetPtr_(nullptr)
{
    setPrimitiveType(GL_LINE_STRIP);
}

BottomTrack::~BottomTrack()
{

}

SceneObject::SceneObjectType BottomTrack::type() const
{
    return SceneObject::SceneObjectType::BottomTrack;
}

bool BottomTrack::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);
    if (m_view->m_mode == GraphicsScene3dView::ActiveMode::Idle) {
        return false;
    }
    if (event->type() == EpochSelected2d) {
        auto epochEvent = static_cast<EpochEvent*>(event);
        resetVertexSelection();
        m_view->m_mode = GraphicsScene3dView::ActiveMode::BottomTrackVertexSelectionMode;
        selectEpoch(epochEvent->epochIndex(), epochEvent->channel().channelId_);
        m_view->update();
    }
    return false;
}

//QList<Epoch*> BottomTrack::epochs() const
//{
//    return m_epochList;
//}

//QMap<ChannelId, DatasetChannel> BottomTrack::channels() const
//{
//    return datasetPtr_->channelsList();
//}

//DatasetChannel BottomTrack::visibleChannel() const
//{
//    return visibleChannel_;
//}

void BottomTrack::setDatasetPtr(Dataset* datasetPtr) {
    datasetPtr_ = datasetPtr;
}

void BottomTrack::setDataProcessorPtr(DataProcessor* dataProcessorPtr)
{
    dataProcessorPtr_ = dataProcessorPtr;
}

void BottomTrack::actionEvent(ActionEvent actionEvent)
{
    auto minMaxFunc = [this](bool isMin) -> void {
        const auto indices{ RENDER_IMPL(BottomTrack)->selectedVertexIndices_ };
        if (!indices.isEmpty()) {
            QVector<int> sequenceVector;
            sequenceVector.reserve(indices.size());

            for (const auto& verticeIndex : indices) {
                const auto epochIndex{ vertex2Epoch_.value(verticeIndex) };
                if (auto epoch{ datasetPtr_->fromIndex(epochIndex) }; epoch) {
                    sequenceVector.push_back(epochIndex);

                    const float coeff = isMin ? 1.1f : 0.9f;
                    const auto channels = datasetPtr_->channelsList();
                    for (const auto& channel : channels) {
                        if (!isMin) {
                            epoch->setMaxDistProc(channel.channelId_, epoch->distProccesing(channel.channelId_) * coeff);
                        }
                        else {
                            epoch->setMinDistProc(channel.channelId_, epoch->distProccesing(channel.channelId_) * coeff);
                        }
                    }
                }
            }

            sequenceVector.shrink_to_fit();
            const auto subArraysVec{ getSubarrays(sequenceVector) };
            const auto channels = datasetPtr_->channelsList();
            const auto btP = datasetPtr_->getBottomTrackParam();

            for (auto& itm : subArraysVec) {
                if (auto* btp = datasetPtr_->getBottomTrackParamPtr(); btp) {
                    btp->indexFrom = itm.first;
                    btp->indexTo = itm.second;

                    if (channels.size() >= 2) { // TODO
                        QMetaObject::invokeMethod(dataProcessorPtr_, "bottomTrackProcessing", Qt::QueuedConnection,
                                                  Q_ARG(DatasetChannel, channels[0]),
                                                  Q_ARG(DatasetChannel, channels[1]),
                                                  Q_ARG(BottomTrackParam, btP),
                                                  Q_ARG(bool, true),/*manual*/
                                                  Q_ARG(bool, false)/*redraw all*/);
                    }
                    else if (channels.size() == 1) {
                        QMetaObject::invokeMethod(dataProcessorPtr_, "bottomTrackProcessing", Qt::QueuedConnection,
                                                  Q_ARG(DatasetChannel, channels[0]),
                                                  Q_ARG(DatasetChannel, DatasetChannel()),
                                                  Q_ARG(BottomTrackParam, btP),
                                                  Q_ARG(bool, true),/*manual*/
                                                  Q_ARG(bool, false));
                    }
                }
                else {
                    break;
                }
            }

            updateRenderData(0, 0, false, true);
            emit datasetPtr_->dataUpdate();
        }
    };

    switch (actionEvent) {
    case ActionEvent::Undefined: {
        break;
    }
    case ActionEvent::ClearDistProc: {
        const auto indices{ RENDER_IMPL(BottomTrack)->selectedVertexIndices_ };

        if (!indices.isEmpty()) {
            bool isSomethingDeleted{ false };
            for (const auto& verticeIndex : indices) {
                const auto epochIndex{ vertex2Epoch_.value(verticeIndex) };
                if (auto epoch{ datasetPtr_->fromIndex(epochIndex) }) {
                    const auto channels = datasetPtr_->channelsList(); // TODO
                    for (const auto& itm : channels) {
                        epoch->clearDistProcessing(itm.channelId_);
                    }

                    Q_EMIT epochErased(epochIndex);
                    isSomethingDeleted = true;
                }
            }
            if (isSomethingDeleted) {
                RENDER_IMPL(BottomTrack)->selectedVertexIndices_.clear();
                updateRenderData(0, 0, false, true);
                emit datasetPtr_->dataUpdate();
            }
        }

        break;
    }
    case ActionEvent::MaxDistProc: {
        minMaxFunc(false);
        break;
    }
    case ActionEvent::MinDistProc: {
        minMaxFunc(true);
        break;
    }
    default:
        break;
    }
}

void BottomTrack::isEpochsChanged(int lEpoch, int rEpoch, bool manual, bool redrawAll)
{
    if (datasetPtr_ && datasetPtr_->getLastBottomTrackEpoch() != 0) {
        auto datasetChannels = datasetPtr_->channelsList();
        if (!datasetChannels.isEmpty()) {
            visibleChannel_ = datasetChannels.first(); // ?!
        }
        else {
            visibleChannel_ = DatasetChannel();
        }

        updateRenderData(lEpoch, rEpoch, redrawAll, manual);

        Q_EMIT epochListChanged();
    }
}

void BottomTrack::setData(const QVector<QVector3D> &data, int primitiveType)
{
    if (m_filter) {
        QVector <QVector3D> filteredData;
        m_filter->apply(data, filteredData);
        SceneObject::setData(filteredData, primitiveType);
        return;
    }

    SceneObject::setData(data, primitiveType);
}

void BottomTrack::clearData()
{
    vertex2Epoch_.clear();
    epoch2Vertex_.clear();
    visibleChannel_ = DatasetChannel();

    auto r = RENDER_IMPL(BottomTrack);
    r->selectedVertexIndices_.clear();

    SceneObject::clearData();
}

void BottomTrack::resetVertexSelection()
{
    RENDER_IMPL(BottomTrack)->selectedVertexIndices_.clear();
}

//void BottomTrack::setVisibleChannel(const ChannelId& channelId)
//{
//    if (!datasetPtr_->channelsList().contains(channelId))
//        return;
//
//    visibleChannel_ = datasetPtr_->channelsList().value(channelId);
//
//    updateRenderData();
//
//    //Q_EMIT visibleChannelChanged(channelId);
//    //Q_EMIT visibleChannelChanged(visibleChannel_);
//    Q_EMIT changed();
//}
//
//void BottomTrack::setVisibleChannel(const DatasetChannel &channel)
//{
//    visibleChannel_ = channel;
//}

void BottomTrack::selectEpoch(int epochIndex, const ChannelId& channelId)
{
    if (m_view->m_mode != GraphicsScene3dView::BottomTrackVertexSelectionMode)
        return;


    if (!datasetPtr_->isContainsChannelInChannelSetup(channelId) || channelId != visibleChannel_.channelId_)
        return;

    if (epochIndex < 0 || epochIndex >= datasetPtr_->size())
        return;

    auto* epoch = datasetPtr_->fromIndex(epochIndex);
    NED nedPos = epoch->getSonarPosition().ned;

    auto indxFromMap = vertex2Epoch_.key(epochIndex);

    if (!epoch ||
        !nedPos.isCoordinatesValid() ||
        (epochIndex && !indxFromMap)) {
        //qDebug() << "invalid pos on bottom track" << epochIndex << indxFromMap;
        return;
    }

    auto r = RENDER_IMPL(BottomTrack);

    r->selectedVertexIndices_.clear();
    r->selectedVertexIndices_.append(indxFromMap);

    Q_EMIT changed();
}

void BottomTrack::setVisibleState(bool state)
{
    SceneObject::setVisible(state);

    m_view->getBoatTrackPtr()->setBottomTrackVisibleState(state);
}

void BottomTrack::mouseMoveEvent(Qt::MouseButtons buttons, qreal x, qreal y)
{
    Q_UNUSED(buttons)
    Q_UNUSED(x)
    Q_UNUSED(y)

    if (!m_view)
        return;

    if (m_view->m_mode == GraphicsScene3dView::BottomTrackVertexSelectionMode) {
        if (buttons.testFlag(Qt::LeftButton)) {
            auto hits = m_view->m_ray.hitObject(shared_from_this(), Ray::HittingMode::Vertex);

            if (!hits.isEmpty()) {
                RENDER_IMPL(BottomTrack)->selectedVertexIndices_ = {hits.first().indices().first};
                auto epochIndex = vertex2Epoch_.value({hits.first().indices().first});

                auto epochEvent = new EpochEvent(EpochSelected3d, datasetPtr_->fromIndex(epochIndex),epochIndex, visibleChannel_);

                QCoreApplication::postEvent(this, epochEvent);
            }
        }
    }

    if(m_view->m_mode == GraphicsScene3dView::BottomTrackVertexComboSelectionMode) {
        RENDER_IMPL(BottomTrack)->selectedVertexIndices_.clear();
        for (int i = 0; i < RENDER_IMPL(BottomTrack)->m_data.size(); i++) {
            auto p = RENDER_IMPL(BottomTrack)->m_data.at(i);
            auto p_screen = p.project(m_view->camera().lock()->viewMatrix()*m_view->m_model,
                            m_view->m_projection,
                            m_view->boundingRect().toRect());

            if (m_view->m_comboSelectionRect.contains(p_screen.x(), p_screen.y()))
                RENDER_IMPL(BottomTrack)->selectedVertexIndices_.append(i);
        }
    }
}

void BottomTrack::mousePressEvent(Qt::MouseButtons buttons, qreal x, qreal y)
{
    Q_UNUSED(buttons)
    Q_UNUSED(x)
    Q_UNUSED(y)

    if (!m_view)
        return;

    if (m_view->m_mode == GraphicsScene3dView::BottomTrackVertexSelectionMode) {
        if (buttons.testFlag(Qt::LeftButton)) {
            auto hits = m_view->m_ray.hitObject(shared_from_this(), Ray::HittingMode::Vertex);
            if (!hits.isEmpty()) {
                auto* r = RENDER_IMPL(BottomTrack);

                r->selectedVertexIndices_ = {hits.first().indices().first};
                auto epochIndex = vertex2Epoch_.value({hits.first().indices().first});
                m_view->getBoatTrackPtr()->selectEpoch(epochIndex);
                auto epochEvent = new EpochEvent(EpochSelected3d, datasetPtr_->fromIndex(epochIndex),epochIndex, visibleChannel_);
                QCoreApplication::postEvent(this, epochEvent);
            }
        }
        /*
        if(!RENDER_IMPL(BottomTrack)->m_selectedVertexIndices.isEmpty()){
            auto epochIndex = m_epochIndexMatchingMap.value(
                        RENDER_IMPL(BottomTrack)->m_selectedVertexIndices.first());

            Q_EMIT epochPressed(epochIndex);
        }
        */
    }
}

void BottomTrack::mouseReleaseEvent(Qt::MouseButtons buttons, qreal x, qreal y)
{
    Q_UNUSED(buttons)
    Q_UNUSED(x)
    Q_UNUSED(y)

    if(!m_view) return;

    auto epochEvent = new EpochEvent(EpochSelected3d, datasetPtr_->fromIndex(-1),-1, visibleChannel_);
    QCoreApplication::postEvent(this, epochEvent);
}

void BottomTrack::keyPressEvent(Qt::Key key)
{
    if (!m_view || !visibleChannel_.channelId_.isValid())
        return;

    if (m_view->m_mode == GraphicsScene3dView::BottomTrackVertexSelectionMode && key == Qt::Key_Delete) {
        const auto indices{ RENDER_IMPL(BottomTrack)->selectedVertexIndices_ };
        bool isSomethingDeleted{ false };
        for (const auto& verticeIndex : indices) {
            const auto epochIndx{ vertex2Epoch_.value(verticeIndex) };
            if (auto epoch{ datasetPtr_->fromIndex(epochIndx) }) {
                const auto channels = datasetPtr_->channelsList(); // TODO
                for (const auto& itm : channels) {
                    epoch->clearDistProcessing(itm.channelId_);
                }

                epoch->clearDistProcessing(visibleChannel_.channelId_);
                Q_EMIT epochErased(epochIndx);
                isSomethingDeleted = true;
            }
        }
        if (isSomethingDeleted) {
            RENDER_IMPL(BottomTrack)->selectedVertexIndices_.clear();
            updateRenderData(0, 0, false, true);
            emit datasetPtr_->dataUpdate();
        }
    }

    if (key == Qt::Key_Delete) {
        const auto indices{ RENDER_IMPL(BottomTrack)->selectedVertexIndices_ };
        if (!indices.isEmpty()) {
            bool isSomethingDeleted{ false };
            for (const auto& verticeIndex : indices) {
                const auto epochIndx{ vertex2Epoch_.value(verticeIndex) };
                if (auto epoch{ datasetPtr_->fromIndex(epochIndx) }) {
                    const auto channels = datasetPtr_->channelsList(); // TODO
                    for (const auto& itm : channels) {
                        epoch->clearDistProcessing(itm.channelId_);
                    }
                    Q_EMIT epochErased(epochIndx);
                    isSomethingDeleted = true;
                }
            }
            if (isSomethingDeleted) {
                RENDER_IMPL(BottomTrack)->selectedVertexIndices_.clear();
                updateRenderData(0, 0, false, true);
                emit datasetPtr_->dataUpdate();
            }
        }
    }
}

void BottomTrack::updateRenderData(int lEpIndx, int rEpIndx, bool redraw, bool manually) //
{
    if (!datasetPtr_) {
        return;
    }

    bool redrawAll = redraw;
    if ((!lEpIndx && !rEpIndx) || (lEpIndx == 0 && rEpIndx == datasetPtr_->size())) {
        redrawAll = true;
    }

    if (redrawAll) {
        clearCache();
    }

    const int toIndx   = redrawAll ? datasetPtr_->getLastBottomTrackEpoch() : rEpIndx;
    const int fromIndx = redrawAll ?                                      0 : lEpIndx;
    if (!redrawAll && fromIndx >= toIndx) {
        return;
    }

    const int need = toIndx - fromIndx;

    auto* r = RENDER_IMPL(BottomTrack);
    r->selectedVertexIndices_.clear(); // ?
    int rSize = r->cdata().size();

    QVector<QVector3D> prepData;
    QVector<int>       epIndxUpdated;
    QVector<int>       vertIndxUpdated;
    prepData.reserve(need);
    epIndxUpdated.reserve(need);
    vertIndxUpdated.reserve(need);

    for (int epIndx = fromIndx; epIndx < toIndx; ++epIndx) {

        auto vIt = epoch2Vertex_.find(epIndx);
        if (vIt != epoch2Vertex_.end()) {
            if (auto* ep = datasetPtr_->fromIndex(epIndx); ep) {
                if (auto pos = ep->getSonarPosition().ned; pos.isCoordinatesValid()) {
                    auto vIndx = *vIt;
                    const float dist = -1.f * static_cast<float>(ep->distProccesing(visibleChannel_.channelId_));
                    r->m_data[vIndx].setZ(dist);

                    epIndxUpdated.push_back(epIndx);
                    vertIndxUpdated.push_back(vIndx);
                }
            }
        }
        else {
            if (auto* ep = datasetPtr_->fromIndex(epIndx); ep) {
                if (auto pos = ep->getSonarPosition().ned; pos.isCoordinatesValid()) {
                    const float dist = -1.f * static_cast<float>(ep->distProccesing(visibleChannel_.channelId_));
                    prepData.push_back(QVector3D(pos.n, pos.e, dist));

                    epIndxUpdated.push_back(epIndx);
                    vertIndxUpdated.push_back(rSize);

                    vertex2Epoch_.insert(rSize, epIndx);
                    epoch2Vertex_.insert(epIndx, rSize);

                    rSize++;
                }
            }
        }
    }

    emit updatedPoints(epIndxUpdated, vertIndxUpdated, manually); // for dataHorizon -> dataProcessor
    SceneObject::appendData(prepData);
}

QVector<QPair<int, int>> BottomTrack::getSubarrays(const QVector<int>& sequenceVector)
{
    QVector<QPair<int, int>> retVal;

    if (sequenceVector.isEmpty()) {
        return retVal;
    }

    int start = sequenceVector[0];
    int end = sequenceVector[0];

    for (int i = 1; i < sequenceVector.size(); ++i) {
        if (sequenceVector[i] == end + 1) {
            end = sequenceVector[i];
        } else {
            retVal.append(qMakePair(start, end));
            start = sequenceVector[i];
            end = sequenceVector[i];
        }
    }

    retVal.append(qMakePair(start, end));

    return retVal;
}

void BottomTrack::clearCache()
{
    auto* r = RENDER_IMPL(BottomTrack);

    r->m_data.clear();
    vertex2Epoch_.clear();
    epoch2Vertex_.clear();
}

//-----------------------RenderImplementation-----------------------------//
BottomTrack::BottomTrackRenderImplementation::BottomTrackRenderImplementation()
{}

BottomTrack::BottomTrackRenderImplementation::~BottomTrackRenderImplementation()
{}

void BottomTrack::BottomTrackRenderImplementation::render(QOpenGLFunctions *ctx,
                                                          const QMatrix4x4 &mvp,
                                                          const QMap<QString,
                                                          std::shared_ptr<QOpenGLShaderProgram> > &shaderProgramMap) const
{
    Q_UNUSED(ctx);
    Q_UNUSED(mvp);
    Q_UNUSED(shaderProgramMap);
}

void BottomTrack::BottomTrackRenderImplementation::render(QOpenGLFunctions *ctx,
                                                          const QMatrix4x4 &model,
                                                          const QMatrix4x4 &view,
                                                          const QMatrix4x4 &projection,
                                                          const QMap<QString,std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible || !shaderProgramMap.contains("height") || !shaderProgramMap.contains("static"))
        return;

    QOpenGLShaderProgram* shaderProgram = nullptr;
    int colorLoc = -1, posLoc = -1, maxZLoc = -1, minZLoc = -1, matrixLoc = -1;

    shaderProgram = shaderProgramMap["height"].get();
    shaderProgram->bind();
    maxZLoc = shaderProgram->uniformLocation("max_z");
    minZLoc = shaderProgram->uniformLocation("min_z");
    shaderProgram->setUniformValue(maxZLoc, m_bounds.maximumZ());
    shaderProgram->setUniformValue(minZLoc, m_bounds.minimumZ());

    posLoc = shaderProgram->attributeLocation("position");
    matrixLoc = shaderProgram->uniformLocation("matrix");

    shaderProgram->setUniformValue(matrixLoc, projection * view * model);
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, m_data.constData());

    ctx->glLineWidth(4.0);
    ctx->glDrawArrays(m_primitiveType, 0, m_data.size());
    ctx->glLineWidth(1.0);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();

    //------------->Drawing selected vertices<<---------------//
    shaderProgram = shaderProgramMap["static"].get();
    shaderProgram->bind();

    colorLoc  = shaderProgram->uniformLocation("color");
    matrixLoc = shaderProgram->uniformLocation("matrix");
    posLoc    = shaderProgram->attributeLocation("position");
    int widthLoc  = shaderProgram->uniformLocation("width");

    QVector4D vertexColor(0.91f, 0.25f, 0.2f, 1.0f);

    //TODO: Needs to optimize data preparing
    QVector<QVector3D> selectedVertices;
    selectedVertices.reserve(selectedVertices.size());
    for (const auto& i : selectedVertexIndices_)
        selectedVertices.append(m_data.at(i));

    if (selectedVertices.isEmpty())
        return;

    shaderProgram->setUniformValue(colorLoc,vertexColor);
    shaderProgram->setUniformValue(matrixLoc, projection * view * model);
    shaderProgram->setUniformValue(widthLoc, 12.0f);
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, selectedVertices.constData());

    ctx->glEnable(34370);
    ctx->glDrawArrays(GL_POINTS, 0, selectedVertices.size());
    ctx->glDisable(34370);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();

    //if (selectedVertices.size() > 1)
    //    return;

    //QRectF vport = DrawUtils::viewportRect(ctx);

    //QVector3D p = selectedVertices.first();
    //QVector2D p_screen = p.project(view * model, projection, vport.toRect()).toVector2D();
    //p_screen.setY(vport.height() - p_screen.y());

    //QMatrix4x4 textProjection;
    //textProjection.ortho(vport.toRect());

    //TextRenderer::instance().render(QString("x=%1 y=%2 z=%3").arg(p.x()).arg(p.y()).arg(p.z()), TODO
    //                                0.3f,
    //                                p_screen,
    //                                ctx,
    //                                textProjection
    //                                );
}
