#include "bottomtrack.h"
#include <graphicsscene3dview.h>
#include <epochevent.h>
//#include <textrenderer.h> TODO
#include <draw_utils.h>
#include "boattrack.h"
#include <QOpenGLFunctions>

#include <QHash>

BottomTrack::BottomTrack(GraphicsScene3dView* view, QObject* parent) :
    SceneObject(new BottomTrackRenderImplementation, view, parent),
    datasetPtr_(nullptr)
{

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

    if (event->type() == EpochSelected2d) {
        auto epochEvent = static_cast<EpochEvent*>(event);
        resetVertexSelection();
        m_view->m_mode = GraphicsScene3dView::ActiveMode::BottomTrackVertexSelectionMode;
        selectEpoch(epochEvent->epochIndex(),epochEvent->channel().channel);
        m_view->update();
    }
    return false;
}

//QList<Epoch*> BottomTrack::epochs() const
//{
//    return m_epochList;
//}

QMap<int, DatasetChannel> BottomTrack::channels() const
{
    return datasetPtr_->channelsList();
}

DatasetChannel BottomTrack::visibleChannel() const
{
    return visibleChannel_;
}

void BottomTrack::setDatasetPtr(Dataset* datasetPtr) {
    datasetPtr_ = datasetPtr;
}

void BottomTrack::actionEvent(ActionEvent actionEvent)
{
    auto minMaxFunc = [this](bool isMin) -> void {
        const auto indices{ RENDER_IMPL(BottomTrack)->selectedVertexIndices_ };
        if (!indices.isEmpty()) {
            QVector<int> sequenceVector;
            sequenceVector.reserve(indices.size());

            for (const auto& verticeIndex : indices) {
                const auto epochIndex{ epochIndexMatchingMap_.value(verticeIndex) };
                if (auto epoch{ datasetPtr_->fromIndex(epochIndex) }; epoch) {
                    sequenceVector.push_back(epochIndex);

                    const float coeff = isMin ? 1.1f : 0.9f;
                    const auto channels = datasetPtr_->channelsList();
                    for (const auto& channel : channels) {
                        if (!isMin) {
                            epoch->setMaxDistProc(channel.channel, epoch->distProccesing(channel.channel) * coeff);
                        }
                        else {
                            epoch->setMinDistProc(channel.channel, epoch->distProccesing(channel.channel) * coeff);
                        }
                    }
                }
            }

            sequenceVector.shrink_to_fit();
            const auto subArraysVec{ getSubarrays(sequenceVector) };

            for (auto& itm : subArraysVec) {
                if (auto btp = datasetPtr_->getBottomTrackParamPtr(); btp) {
                    btp->indexFrom = itm.first;
                    btp->indexTo = itm.second;
                    datasetPtr_->bottomTrackProcessing(0, 1); // TODO: 0, 1
                }
                else {
                    break;
                }
            }

            updateRenderData();
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
                const auto epochIndex{ epochIndexMatchingMap_.value(verticeIndex) };
                if (auto epoch{ datasetPtr_->fromIndex(epochIndex) }) {
                    epoch->clearDistProcessing(visibleChannel_.channel);
                    Q_EMIT epochErased(epochIndex);
                    isSomethingDeleted = true;
                }
            }
            if (isSomethingDeleted) {
                RENDER_IMPL(BottomTrack)->selectedVertexIndices_.clear();
                updateRenderData();
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

void BottomTrack::isEpochsChanged(int lEpoch, int rEpoch)
{
    if (datasetPtr_ && datasetPtr_->getLastBottomTrackEpoch() != 0) {
        auto channelMap = datasetPtr_->channelsList();
        if (!channelMap.isEmpty()) {
            if (visibleChannel_.channel < channelMap.first().channel ||
                visibleChannel_.channel > channelMap.last().channel)
                visibleChannel_ = channelMap.first();
        }
        else
            visibleChannel_ = DatasetChannel();

        updateRenderData(lEpoch, rEpoch);
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
    epochIndexMatchingMap_.clear();
    renderData_.clear();
    visibleChannel_ = DatasetChannel();

    auto r = RENDER_IMPL(BottomTrack);
    r->selectedVertexIndices_.clear();

    r->surfaceUpdated_ = false;
    r->sideScanUpdated_ = false;

    SceneObject::clearData();
}

void BottomTrack::resetVertexSelection()
{
    RENDER_IMPL(BottomTrack)->selectedVertexIndices_.clear();
}

void BottomTrack::setVisibleChannel(int channelId)
{
    if (!datasetPtr_->channelsList().contains(channelId))
        return;

    visibleChannel_ = datasetPtr_->channelsList().value(channelId);

    updateRenderData();

    Q_EMIT visibleChannelChanged(channelId);
    Q_EMIT visibleChannelChanged(visibleChannel_);
    Q_EMIT changed();
}

void BottomTrack::setVisibleChannel(const DatasetChannel &channel)
{
    visibleChannel_ = channel;
}

void BottomTrack::selectEpoch(int epochIndex, int channelId)
{
    if (m_view->m_mode != GraphicsScene3dView::BottomTrackVertexSelectionMode)
        return;

    if (!datasetPtr_->channelsList().contains(channelId) || channelId != visibleChannel_.channel)
        return;

    if (epochIndex < 0 || epochIndex >= datasetPtr_->size())
        return;

    auto* epoch = datasetPtr_->fromIndex(epochIndex);
    auto indxFromMap = epochIndexMatchingMap_.key(epochIndex);

    if (!epoch ||
        !epoch->getPositionGNSS().ned.isCoordinatesValid() ||
        (epochIndex && !indxFromMap)) {
        return;
    }

    auto r = RENDER_IMPL(BottomTrack);

    r->selectedVertexIndices_.clear();
    r->selectedVertexIndices_.append(indxFromMap);

    Q_EMIT changed();
}

void BottomTrack::surfaceUpdated()
{
    RENDER_IMPL(BottomTrack)->surfaceUpdated_ = true;
}

void BottomTrack::sideScanUpdated()
{
    RENDER_IMPL(BottomTrack)->sideScanUpdated_ = true;
}

void BottomTrack::surfaceStateChanged(bool state)
{
    RENDER_IMPL(BottomTrack)->surfaceState_ = state;
}

void BottomTrack::setSideScanVisibleState(bool state)
{
    RENDER_IMPL(BottomTrack)->sideScanVisibleState_ = state;
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
                auto epochIndex = epochIndexMatchingMap_.value({hits.first().indices().first});

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
                RENDER_IMPL(BottomTrack)->selectedVertexIndices_ = {hits.first().indices().first};
                auto epochIndex = epochIndexMatchingMap_.value({hits.first().indices().first});
                m_view->boatTrack()->selectEpoch(epochIndex);
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
    if (!m_view || visibleChannel_.channel < 0)
        return;

    if (m_view->m_mode == GraphicsScene3dView::BottomTrackVertexSelectionMode && key == Qt::Key_Delete) {
        const auto indices{ RENDER_IMPL(BottomTrack)->selectedVertexIndices_ };
        bool isSomethingDeleted{ false };
        for (const auto& verticeIndex : indices) {
            const auto epochIndx{ epochIndexMatchingMap_.value(verticeIndex) };
            if (auto epoch{ datasetPtr_->fromIndex(epochIndx) }) {
                epoch->clearDistProcessing(visibleChannel_.channel);
                Q_EMIT epochErased(epochIndx);
                isSomethingDeleted = true;
            }
        }
        if (isSomethingDeleted) {
            RENDER_IMPL(BottomTrack)->selectedVertexIndices_.clear();
            updateRenderData();
            emit datasetPtr_->dataUpdate();
        }
    }

    if (key == Qt::Key_Delete) {
        const auto indices{ RENDER_IMPL(BottomTrack)->selectedVertexIndices_ };
        if (!indices.isEmpty()) {
            bool isSomethingDeleted{ false };
            for (const auto& verticeIndex : indices) {
                const auto epochIndex{ epochIndexMatchingMap_.value(verticeIndex) };
                if (auto epoch{ datasetPtr_->fromIndex(epochIndex) }) {
                    epoch->clearDistProcessing(visibleChannel_.channel);
                    Q_EMIT epochErased(epochIndex);
                    isSomethingDeleted = true;
                }
            }
            if (isSomethingDeleted) {
                RENDER_IMPL(BottomTrack)->selectedVertexIndices_.clear();
                updateRenderData();
                emit datasetPtr_->dataUpdate();
            }
        }
    }
}

void BottomTrack::updateRenderData(int lEpoch, int rEpoch)
{
    bool interCall = (rEpoch == 0) && (lEpoch == 0);
    bool updateAll = (rEpoch - lEpoch) == datasetPtr_->getLastBottomTrackEpoch();
    bool defMode = interCall || updateAll;

    RENDER_IMPL(BottomTrack)->selectedVertexIndices_.clear();

    if (defMode) {
        epochIndexMatchingMap_.clear();
        renderData_.clear();
    }

    bool beenUpdated{ false };
    auto appendData = [&, this](Position& pos, float distance, int i) ->void {
        renderData_.append(QVector3D(pos.ned.n, pos.ned.e, distance));
        epochIndexMatchingMap_.insert(renderData_.size() - 1, i);
        beenUpdated = true;
    };

    if (visibleChannel_.channel > -1) {
        int currMin = defMode ? 0 : lEpoch;
        int currMax = defMode ? datasetPtr_->getLastBottomTrackEpoch() : rEpoch;
        if (defMode)
            renderData_.reserve(currMax);
        for (int i = currMin; i < currMax; ++i) {
            auto epoch = datasetPtr_->fromIndex(i);
            if (!epoch)
                continue;

            auto pos = epoch->getPositionGNSS();

            if (pos.ned.isCoordinatesValid()) {
                float distance = -1.f * static_cast<float>(epoch->distProccesing(visibleChannel_.channel));
                if (defMode) {
                    appendData(pos, distance, i);
                }
                else {
                    for (int j = 0; j < renderData_.size(); ++j) { // first - find correct point by pos
                        if (renderData_[j].x() == static_cast<float>(pos.ned.n) &&
                            renderData_[j].y() == static_cast<float>(pos.ned.e)) {
                            renderData_[j] = QVector3D(pos.ned.n, pos.ned.e, distance);
                            epochIndexMatchingMap_[j] = i;
                            beenUpdated = true;
                            break;
                        }
                    }
                    if (!beenUpdated) { // rewrite cause we have new undefined epoch (undef pos left, mid, right)
                        epochIndexMatchingMap_.clear();
                        renderData_.clear();
                        int cCurrMin = 0;
                        int cCurrMax = datasetPtr_->getLastBottomTrackEpoch();
                        renderData_.reserve(cCurrMax);
                        for (int k = cCurrMin; k < cCurrMax; ++k) {
                            if (auto cEpoch = datasetPtr_->fromIndex(k); cEpoch) {
                                auto cPos = cEpoch->getPositionGNSS();
                                if (cPos.ned.isCoordinatesValid()) {
                                    float cDistance = -1.f * static_cast<float>(cEpoch->distProccesing(visibleChannel_.channel));
                                    appendData(cPos, cDistance, k);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (beenUpdated && !renderData_.empty()) {
        SceneObject::setData(renderData_, GL_LINE_STRIP);
    }
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

//-----------------------RenderImplementation-----------------------------//
BottomTrack::BottomTrackRenderImplementation::BottomTrackRenderImplementation() :
    surfaceUpdated_(false),
    sideScanUpdated_(false),
    surfaceState_(true),
    sideScanVisibleState_(true)
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

    if ((surfaceUpdated_ && surfaceState_) || (sideScanUpdated_ && sideScanVisibleState_)) {
        shaderProgram = shaderProgramMap["static"].get();
        shaderProgram->bind();
        colorLoc = shaderProgram->uniformLocation("color");
        shaderProgram->setUniformValue(colorLoc,QVector4D(1.0f, 0.2f, 0.2f, 1.0f));
    }
    else {
        shaderProgram = shaderProgramMap["height"].get();
        shaderProgram->bind();
        maxZLoc = shaderProgram->uniformLocation("max_z");
        minZLoc = shaderProgram->uniformLocation("min_z");
        shaderProgram->setUniformValue(maxZLoc, m_bounds.maximumZ());
        shaderProgram->setUniformValue(minZLoc, m_bounds.minimumZ());
    }

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

    QVector4D vertexColor(0.03f, 0.69f, 0.98f, 1.0f);

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
