#include "bottomtrack.h"
#include <graphicsscene3dview.h>
#include <epochevent.h>
#include <textrenderer.h>
#include <drawutils.h>
#include <QtOpenGLExtensions/QOpenGLExtensions>

#include <QHash>

BottomTrack::BottomTrack(GraphicsScene3dView* view, QObject* parent)
    : SceneObject(new BottomTrackRenderImplementation,view,parent), datasetPtr_(nullptr)
{}

BottomTrack::~BottomTrack()
{}

SceneObject::SceneObjectType BottomTrack::type() const
{
    return SceneObject::SceneObjectType::BottomTrack;
}

bool BottomTrack::eventFilter(QObject *watched, QEvent *event)
{
    if(event->type() == EpochSelected2d){
        auto epochEvent = static_cast<EpochEvent*>(event);
        selectEpoch(epochEvent->epochIndex(),epochEvent->channel().channel);
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
    return m_visibleChannel;
}

void BottomTrack::setDatasetPtr(Dataset* datasetPtr) {
    datasetPtr_ = datasetPtr;
}

void BottomTrack::isEpochsChanged()
{
    if(datasetPtr_ && !datasetPtr_->channelsList().isEmpty()){
        if(m_visibleChannel.channel < datasetPtr_->channelsList().first().channel ||
           m_visibleChannel.channel > datasetPtr_->channelsList().last().channel)
           m_visibleChannel = datasetPtr_->channelsList().first();
    }
    else
        m_visibleChannel = DatasetChannel();

    updateRenderData();

    Q_EMIT epochListChanged();
}

void BottomTrack::setData(const QVector<QVector3D> &data, int primitiveType)
{
    if(m_filter){
        QVector <QVector3D> filteredData;
        m_filter->apply(data, filteredData);
        SceneObject::setData(filteredData, primitiveType);
        return;
    }

    SceneObject::setData(data, primitiveType);
}

void BottomTrack::clearData()
{
    m_llaRef.isInit = false;
    m_epochIndexMatchingMap.clear();
    m_visibleChannel = DatasetChannel();
    SceneObject::clearData();
}

void BottomTrack::resetVertexSelection()
{
    RENDER_IMPL(BottomTrack)->m_selectedVertexIndices.clear();
}

void BottomTrack::setDisplayingWithSurface(bool displaying)
{
    RENDER_IMPL(BottomTrack)->m_isDisplayingWithSurface = displaying;
}

void BottomTrack::setVisibleChannel(int channelId)
{
    if(!datasetPtr_->channelsList().contains(channelId))
        return;

    m_visibleChannel = datasetPtr_->channelsList().value(channelId);

    updateRenderData();

    Q_EMIT visibleChannelChanged(channelId);
    Q_EMIT visibleChannelChanged(m_visibleChannel);
    Q_EMIT changed();
}

void BottomTrack::setVisibleChannel(const DatasetChannel &channel)
{
    m_visibleChannel = channel;
}

void BottomTrack::selectEpoch(int epochIndex, int channelId)
{
    if(m_view->m_mode != GraphicsScene3dView::BottomTrackVertexSelectionMode)
        return;

    if(!datasetPtr_->channelsList().contains(channelId) || channelId != m_visibleChannel.channel)
        return;

    if(epochIndex < 0 || epochIndex >= datasetPtr_->size())
        return;

    if(!datasetPtr_->fromIndex(epochIndex))
        return;

    auto r = RENDER_IMPL(BottomTrack);

    r->m_selectedVertexIndices.clear();
    r->m_selectedVertexIndices.append(m_epochIndexMatchingMap.key(epochIndex));

    Q_EMIT changed();
}

void BottomTrack::mouseMoveEvent(Qt::MouseButtons buttons, qreal x, qreal y)
{
    Q_UNUSED(buttons)
    Q_UNUSED(x)
    Q_UNUSED(y)

    if(!m_view) return;

    if(m_view->m_mode == GraphicsScene3dView::BottomTrackVertexSelectionMode){
        if(buttons.testFlag(Qt::LeftButton)){
            auto hits = m_view->m_ray.hitObject(shared_from_this(), Ray::HittingMode::Vertex);

            if(!hits.isEmpty()){
                RENDER_IMPL(BottomTrack)->m_selectedVertexIndices = {hits.first().indices().first};
                auto epochIndex = m_epochIndexMatchingMap.value({hits.first().indices().first});

                auto epochEvent = new EpochEvent(EpochSelected3d, datasetPtr_->fromIndex(epochIndex),epochIndex, m_visibleChannel);

                QCoreApplication::postEvent(this, epochEvent);
            }
        }
    }

    if(m_view->m_mode == GraphicsScene3dView::BottomTrackVertexComboSelectionMode)
    {
        RENDER_IMPL(BottomTrack)->m_selectedVertexIndices.clear();
        for(int i = 0; i < RENDER_IMPL(BottomTrack)->m_data.size(); i++){
            auto p = RENDER_IMPL(BottomTrack)->m_data.at(i);
            auto p_screen = p.project(m_view->camera().lock()->viewMatrix()*m_view->m_model,
                            m_view->m_projection,
                            m_view->boundingRect().toRect());

            if(m_view->m_comboSelectionRect.contains(p_screen.x(), p_screen.y()))
                RENDER_IMPL(BottomTrack)->m_selectedVertexIndices.append(i);
        }
    }
}

void BottomTrack::mousePressEvent(Qt::MouseButtons buttons, qreal x, qreal y)
{
    Q_UNUSED(buttons)
    Q_UNUSED(x)
    Q_UNUSED(y)

    if(!m_view) return;

    if(m_view->m_mode == GraphicsScene3dView::BottomTrackVertexSelectionMode){
        if(!RENDER_IMPL(BottomTrack)->m_selectedVertexIndices.isEmpty()){
            auto epochIndex = m_epochIndexMatchingMap.value(
                        RENDER_IMPL(BottomTrack)->m_selectedVertexIndices.first());

            Q_EMIT epochPressed(epochIndex);
        }
    }
}

void BottomTrack::mouseReleaseEvent(Qt::MouseButtons buttons, qreal x, qreal y)
{
    Q_UNUSED(buttons)
    Q_UNUSED(x)
    Q_UNUSED(y)

    if(!m_view) return;
}

void BottomTrack::keyPressEvent(Qt::Key key)
{
    if(!m_view) return;

    if(m_view->m_mode == GraphicsScene3dView::BottomTrackVertexSelectionMode)
    {
        if(key == Qt::Key_Delete){
            auto indices = RENDER_IMPL(BottomTrack)->m_selectedVertexIndices;

            for(const auto& verticeIndex : indices){
                auto epochIndex = m_epochIndexMatchingMap.value(verticeIndex);
                auto epoch = datasetPtr_->fromIndex(epochIndex);

                if(m_visibleChannel.channel < 0)
                    return;

                epoch->clearDistProcessing(m_visibleChannel.channel);

                Q_EMIT epochErased(epochIndex);
            }

            RENDER_IMPL(BottomTrack)->m_selectedVertexIndices.clear();

            updateRenderData();
        }
    }

    if(m_view->m_mode == GraphicsScene3dView::BottomTrackVertexComboSelectionMode)
    {
        if(key == Qt::Key_Delete){
            auto indices = RENDER_IMPL(BottomTrack)->m_selectedVertexIndices;

            for(const auto& verticeIndex : indices){
                auto epochIndex = m_epochIndexMatchingMap.value(verticeIndex);
                auto epoch = datasetPtr_->fromIndex(epochIndex);

                if(m_visibleChannel.channel < 0)
                    return;

                epoch->clearDistProcessing(m_visibleChannel.channel);

                Q_EMIT epochErased(epochIndex);
            }

            RENDER_IMPL(BottomTrack)->m_selectedVertexIndices.clear();

            m_view->m_comboSelectionRect = {m_view->m_comboSelectionRect.bottomRight(),
                                            m_view->m_comboSelectionRect.bottomRight()};

            updateRenderData();
        }
    }
}

void BottomTrack::updateRenderData()
{
    RENDER_IMPL(BottomTrack)->m_selectedVertexIndices.clear();
    m_epochIndexMatchingMap.clear();
    QVector<QVector3D> renderData;

    if (m_visibleChannel.channel > -1) {
        for (int i = 0; i < datasetPtr_->getLastBottomTrackEpoch(); ++i) {
            auto epoch = datasetPtr_->fromIndex(i);
            if (!epoch)
                continue;
            auto pos = epoch->getPositionGNSS();
            if (pos.lla.isCoordinatesValid() && !pos.ned.isCoordinatesValid()) {
                if(!m_llaRef.isInit)
                    m_llaRef = LLARef(pos.lla);
                pos.LLA2NED(&m_llaRef);
            }
            if (pos.ned.isCoordinatesValid()) {
                float distance = -1.0 * epoch->distProccesing(m_visibleChannel.channel);
                if(!isfinite(distance))
                    continue;
                renderData.append(QVector3D(pos.ned.n,pos.ned.e,distance));
                m_epochIndexMatchingMap.insert(renderData.size() - 1, i);
            }
        }
    }

    if (!renderData.empty())
        SceneObject::setData(renderData,GL_LINE_STRIP);
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

}

void BottomTrack::BottomTrackRenderImplementation::render(QOpenGLFunctions *ctx,
                                                          const QMatrix4x4 &model,
                                                          const QMatrix4x4 &view,
                                                          const QMatrix4x4 &projection,
                                                          const QMap<QString,std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if(!m_isVisible)
        return;

    if(!shaderProgramMap.contains("height"))
        return;

    QOpenGLShaderProgram* shaderProgram = nullptr;
    int colorLoc = -1, posLoc = -1, maxZLoc = -1, minZLoc = -1, matrixLoc = -1;

    if(m_isDisplayingWithSurface){
        shaderProgram = shaderProgramMap["static"].get();
        shaderProgram->bind();
        colorLoc = shaderProgram->uniformLocation("color");
        shaderProgram->setUniformValue(colorLoc,QVector4D(1.0f, 0.2f, 0.2f, 1.0f));
    }else{
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

    QVector4D vertexColor(0.0f, 0.3f, 1.0f, 1.0f);

    //TODO: Needs to optimize data preparing
    QVector<QVector3D> selectedVertices;
    for(const auto& i : m_selectedVertexIndices)
        selectedVertices.append(m_data.at(i));

    if(selectedVertices.isEmpty())
        return;

    shaderProgram->setUniformValue(colorLoc,vertexColor);
    shaderProgram->setUniformValue(matrixLoc, projection * view * model);
    shaderProgram->setUniformValue(widthLoc, 10.0f);
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, selectedVertices.constData());

    ctx->glEnable(GL_PROGRAM_POINT_SIZE);
    ctx->glLineWidth(4.0);
    ctx->glDrawArrays(GL_POINTS, 0, selectedVertices.size());
    ctx->glLineWidth(1.0);
    ctx->glDisable(GL_PROGRAM_POINT_SIZE);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();

    if(selectedVertices.size() > 1)
        return;

    QRectF vport = DrawUtils::viewportRect(ctx);

    QVector3D p = selectedVertices.first();
    QVector2D p_screen = p.project(view * model, projection, vport.toRect()).toVector2D();
    p_screen.setY(vport.height() - p_screen.y());

    QMatrix4x4 textProjection;
    textProjection.ortho(vport.toRect());

    TextRenderer::instance().render(QString("x=%1 y=%2 z=%3").arg(p.x()).arg(p.y()).arg(p.z()),
                                    0.3f,
                                    p_screen,
                                    ctx,
                                    textProjection
                                    );
}
