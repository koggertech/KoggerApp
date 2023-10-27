#include "planegrid.h"
#include <drawutils.h>

PlaneGrid::PlaneGrid(QObject *parent)
    : SceneObject(new PlaneGridRenderImplementation, parent)
{}

void PlaneGrid::setSize(const QSize &size)
{
    RENDER_IMPL(PlaneGrid)->m_size = size;

    Q_EMIT changed();
}

void PlaneGrid::setPosition(const QVector3D &pos)
{
    m_position = pos;
    RENDER_IMPL(PlaneGrid)->m_position = pos;

    Q_EMIT changed();
}

void PlaneGrid::setCellSize(int size)
{
    m_cellSize = size;
    RENDER_IMPL(PlaneGrid)->m_cellSize = size;

    Q_EMIT changed();
}

void PlaneGrid::setData(const QVector<QVector3D> &data, int primitiveType)
{
    Q_UNUSED(data)
    Q_UNUSED(primitiveType)
}

PlaneGrid::PlaneGridRenderImplementation::PlaneGridRenderImplementation()
{}

PlaneGrid::PlaneGridRenderImplementation::~PlaneGridRenderImplementation()
{}

void PlaneGrid::PlaneGridRenderImplementation::render(QOpenGLFunctions *ctx,
                                                      const QMatrix4x4 &mvp,
                                                      const QMap<QString,
                                                      std::shared_ptr<QOpenGLShaderProgram> > &shaderProgramMap) const
{
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


    shaderProgram->setUniformValue(matrixLoc, mvp);
    shaderProgram->enableAttributeArray(posLoc);

    QVector<QVector3D> grid;

    int cellCount = std::round(std::max(m_size.height(), m_size.width()) * 2 / m_cellSize);

    if(cellCount % 2 != 0)
        cellCount++;

    int gridSize = cellCount * m_cellSize;

    for(int i = 0; i <= cellCount; i++){
        grid.append({
                {m_position.x() - static_cast<float>(gridSize) / 2.0f, m_position.y() - static_cast<float>(gridSize) / 2.0f + static_cast<float>(i) * m_cellSize, m_position.z()},
                {m_position.x() + static_cast<float>(gridSize) / 2.0f, m_position.y() - static_cast<float>(gridSize) / 2.0f + static_cast<float>(i) * m_cellSize, m_position.z()},
                {m_position.x() - static_cast<float>(gridSize) / 2.0f + static_cast<float>(i) * m_cellSize, m_position.y() - static_cast<float>(gridSize) / 2.0f, m_position.z()},
                {m_position.x() - static_cast<float>(gridSize) / 2.0f + static_cast<float>(i) * m_cellSize, m_position.y() + static_cast<float>(gridSize) / 2.0f, m_position.z()}
            });
    }

    //QVector<QVector3D> globalGrid;
    //for(float i = m_position.y()-m_size.height()*0.7; i <= m_position.y() + m_size.height()*0.7; i+=m_cellSize){
    //    globalGrid.append({
    //                  {m_position.x()-static_cast<float>(m_size.width()*0.7), i, m_position.z()},
    //                  {m_position.x()+static_cast<float>(m_size.width()*0.7), i, m_position.z()}}
    //                 );
    //}

    //for(float i = m_position.x()-m_size.width()*0.7; i <= m_position.x()+m_size.width()*0.7; i+=m_cellSize){
    //    globalGrid.append({
    //                  {i, m_position.y()-static_cast<float>(m_size.height()*0.7), m_position.z()},
    //                  {i, m_position.y()+static_cast<float>(m_size.height()*0.7), m_position.z()}}
    //                 );
    //}

    //QVector<QVector3D> localGrid;
    //for(float i = m_position.y()-m_size.height()/2.0f; i <= m_position.y() + m_size.height()/2.0f; i+=m_cellSize){
    //    localGrid.append({
    //                  {m_position.x()-static_cast<float>(m_size.width()/2.0f), i, m_position.z()},
    //                  {m_position.x()+static_cast<float>(m_size.width()/2.0f), i, m_position.z()}}
    //                 );
    //}

    //for(float i = m_position.x()-m_size.width()/2.0f; i <= m_position.x()+m_size.width()/2.0f; i+=m_cellSize){
    //    localGrid.append({
    //                  {i, m_position.y()-static_cast<float>(m_size.height()/2.0f), m_position.z()},
    //                  {i, m_position.y()+static_cast<float>(m_size.height()/2.0f), m_position.z()}}
    //                 );
    //}

    QVector<QVector3D> sceneBoundsPlane{
        {m_position.x()-static_cast<float>(m_size.width()/2.0f), m_position.y()-static_cast<float>(m_size.height()/2.0f), m_position.z()},
        {m_position.x()+static_cast<float>(m_size.width()/2.0f), m_position.y()-static_cast<float>(m_size.height()/2.0f), m_position.z()},
        {m_position.x()+static_cast<float>(m_size.width()/2.0f), m_position.y()+static_cast<float>(m_size.height()/2.0f), m_position.z()},
        {m_position.x()-static_cast<float>(m_size.width()/2.0f), m_position.y()+static_cast<float>(m_size.height()/2.0f), m_position.z()},
    };

    ctx->glLineWidth(1.0f);
    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(0.0f, 0.0f, 0.0f, 200.0f)));
    shaderProgram->setAttributeArray(posLoc, grid.constData());
    ctx->glDrawArrays(GL_LINES, 0, grid.size());
    ctx->glLineWidth(1.0f);

    //ctx->glLineWidth(2.0f);
    //shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(0.0f, 200.0f, 255.0f, 200.0f)));
    //shaderProgram->setAttributeArray(posLoc, localGrid.constData());
    //ctx->glDrawArrays(GL_LINES, 0, localGrid.size());
    //ctx->glLineWidth(1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ctx->glLineWidth(4.0f);
    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(0.0f, 200.0f, 255.0f, 200.0f)));
    shaderProgram->setAttributeArray(posLoc, sceneBoundsPlane.constData());
    ctx->glDrawArrays(GL_QUADS, 0, sceneBoundsPlane.size());
    ctx->glLineWidth(1.0f);
    glDisable(GL_BLEND);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}

