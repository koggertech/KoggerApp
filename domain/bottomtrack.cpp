#include "bottomtrack.h"
#include <graphicsscene3dview.h>

BottomTrack::BottomTrack(GraphicsScene3dView* view, QObject* parent)
    : SceneObject(new BottomTrackRenderImplementation,view,parent)
{}

BottomTrack::~BottomTrack()
{}

SceneObject::SceneObjectType BottomTrack::type() const
{
    return SceneObject::SceneObjectType::BottomTrack;
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

void BottomTrack::resetVertexSelection()
{
    RENDER_IMPL(BottomTrack)->m_selectedVertexIndices.clear();
}

void BottomTrack::mouseMoveEvent(Qt::MouseButtons buttons, qreal x, qreal y)
{
    Q_UNUSED(buttons)
    Q_UNUSED(x)
    Q_UNUSED(y)

    if(!m_view) return;

    if(m_view->m_mode == GraphicsScene3dView::BottomTrackVertexSelectionMode){
        auto hits = m_view->m_ray.hitObject(shared_from_this(), Ray::HittingMode::Vertex);
        if(!hits.isEmpty())
            RENDER_IMPL(BottomTrack)->m_selectedVertexIndices = {hits.first().indices().first};
    }

    if(m_view->m_mode == GraphicsScene3dView::BottomTrackVertexComboSelectionMode)
    {
        RENDER_IMPL(BottomTrack)->m_selectedVertexIndices.clear();
        for(int i = 0; i < RENDER_IMPL(BottomTrack)->m_data.size(); i++){
            auto p = RENDER_IMPL(BottomTrack)->m_data.at(i);
            auto p_screen = p.project(m_view->m_model * m_view->camera().lock()->viewMatrix(),
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

            for(const auto& i : indices)
                removeVertex(i);

            RENDER_IMPL(BottomTrack)->m_selectedVertexIndices.clear();
        }
    }

    if(m_view->m_mode == GraphicsScene3dView::BottomTrackVertexComboSelectionMode)
    {
        if(key == Qt::Key_Delete){
            QVector<QVector3D> newData;

            auto indices = RENDER_IMPL(BottomTrack)->m_selectedVertexIndices;
            auto currentData = RENDER_IMPL(BottomTrack)->cdata();

            for(int i = 0; i < currentData.size(); i++){
                if(!indices.contains(i))
                    newData.append(currentData.at(i));
            }
            RENDER_IMPL(BottomTrack)->setData(newData, GL_LINE_STRIP);
            RENDER_IMPL(BottomTrack)->m_selectedVertexIndices.clear();

            m_view->m_comboSelectionRect = {0,0,0,0};
        }
    }
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
    if(!m_isVisible)
        return;

    if(!shaderProgramMap.contains("height"))
        return;

    auto shaderProgram = shaderProgramMap["height"];

    if (!shaderProgram->bind()){
        qCritical() << "Error binding shader program.";
        return;
    }

    int posLoc    = shaderProgram->attributeLocation("position");
    int maxYLoc   = shaderProgram->uniformLocation("max_y");
    int minYLoc   = shaderProgram->uniformLocation("min_y");
    int matrixLoc = shaderProgram->uniformLocation("matrix");

    QVector4D color(0.8f, 0.2f, 0.7f, 1.0f);
    int colorLoc = shaderProgram->uniformLocation("color");

    shaderProgram->setUniformValue(colorLoc,color);
    shaderProgram->setUniformValue(maxYLoc, m_bounds.maximumY());
    shaderProgram->setUniformValue(minYLoc, m_bounds.minimumY());
    shaderProgram->setUniformValue(matrixLoc, mvp);
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, m_data.constData());

    ctx->glLineWidth(4.0);
    ctx->glDrawArrays(m_primitiveType, 0, m_data.size());
    ctx->glLineWidth(1.0);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();

    //------------->Drawing selected vertices<<---------------//
    shaderProgram = shaderProgramMap["static"];
    shaderProgram->bind();

    colorLoc  = shaderProgram->uniformLocation("color");
    matrixLoc = shaderProgram->uniformLocation("matrix");
    posLoc    = shaderProgram->attributeLocation("position");
    int widthLoc  = shaderProgram->uniformLocation("width");

    QVector4D vertexColor(1.0f, 0.0f, 0.0f, 1.0f);
    QVector<QVector3D> selectedVertices;
    for(const auto& i : m_selectedVertexIndices)
        selectedVertices.append(m_data.at(i));

    shaderProgram->setUniformValue(colorLoc,vertexColor);
    shaderProgram->setUniformValue(matrixLoc, mvp);
    shaderProgram->setUniformValue(widthLoc, 10.0f);
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, selectedVertices.constData());

    ctx->glEnable(GL_PROGRAM_POINT_SIZE);
    ctx->glLineWidth(4.0);
    ctx->glDrawArrays(GL_POINTS, 0, selectedVertices.size());
    ctx->glLineWidth(1.0);
    ctx->glDisable(GL_PROGRAM_POINT_SIZE);
    //------------->
}
