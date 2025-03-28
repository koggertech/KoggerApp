#include "plane_grid.h"
#include "draw_utils.h"
//#include <text_renderer.h>

PlaneGrid::PlaneGrid(QObject *parent)
    : SceneObject(new PlaneGridRenderImplementation, parent)
{}

void PlaneGrid::setPlane(const Plane &plane)
{
    auto r = RENDER_IMPL(PlaneGrid);

    r->m_size = plane.size();
    r->m_position = plane.center();

    Q_EMIT changed();
}

void PlaneGrid::setSize(const QSizeF &size)
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

    int cellCount = std::round(std::max(m_size.height(), m_size.width()) * 1.3 / m_cellSize);

    if(cellCount % 2 != 0)
        cellCount++;

    int gridSize = cellCount * m_cellSize;

    for(int i = 0; i <= cellCount; i++){
        grid.append({
                {m_position.x() - static_cast<float>(gridSize) / 2.0f, m_position.y() - static_cast<float>(gridSize) / 2.0f + static_cast<float>(i) * m_cellSize, m_position.z()},
                {m_position.x() + static_cast<float>(gridSize) / 2.0f, m_position.y() - static_cast<float>(gridSize) / 2.0f + static_cast<float>(i) * m_cellSize, m_position.z()},
                {m_position.x() - static_cast<float>(gridSize) / 2.0f + static_cast<float>(i) * m_cellSize,m_position.y() - static_cast<float>(gridSize) / 2.0f, m_position.z()},
                {m_position.x() - static_cast<float>(gridSize) / 2.0f + static_cast<float>(i) * m_cellSize,m_position.y() + static_cast<float>(gridSize) / 2.0f, m_position.z()}
            });
    }

    QVector<QVector3D> sceneBoundsPlane{
        {m_position.x()-static_cast<float>(m_size.height()/2.0f), m_position.y()-static_cast<float>(m_size.width()/2.0f), m_position.z()},
        {m_position.x()+static_cast<float>(m_size.height()/2.0f), m_position.y()-static_cast<float>(m_size.width()/2.0f), m_position.z()},
        {m_position.x()+static_cast<float>(m_size.height()/2.0f), m_position.y()+static_cast<float>(m_size.width()/2.0f), m_position.z()},
        {m_position.x()-static_cast<float>(m_size.height()/2.0f), m_position.y()+static_cast<float>(m_size.width()/2.0f), m_position.z()},
    };

    QVector<QVector3D> horzSizeLine{
        {m_position.x()-static_cast<float>(m_size.height()/2.0f), m_position.y()-static_cast<float>(m_size.width()/2.0f) - m_cellSize, m_position.z()},
        {m_position.x()+static_cast<float>(m_size.height()/2.0f), m_position.y()-static_cast<float>(m_size.width()/2.0f) - m_cellSize, m_position.z()}
    };

    QVector<QVector3D> vertSizeLine{
        {m_position.x()-static_cast<float>(m_size.height()/2.0f) - m_cellSize, m_position.y()-static_cast<float>(m_size.width()/2.0f), m_position.z()},
        {m_position.x()-static_cast<float>(m_size.height()/2.0f) - m_cellSize, m_position.y()+static_cast<float>(m_size.width()/2.0f), m_position.z()},
    };

    QVector<QVector3D> horzReferenceLines{
        sceneBoundsPlane.at(0),
        horzSizeLine.at(0),
        sceneBoundsPlane.at(1),
        horzSizeLine.at(1)
    };
    horzReferenceLines[1].setY(horzReferenceLines[1].y() - static_cast<float>(m_cellSize));
    horzReferenceLines[3].setY(horzReferenceLines[3].y() - static_cast<float>(m_cellSize));

    QVector<QVector3D> vertReferenceLines{
        sceneBoundsPlane.at(1),
        vertSizeLine.at(0),
        sceneBoundsPlane.at(2),
        vertSizeLine.at(1)
    };
    vertReferenceLines[1].setX(vertReferenceLines[1].x() - static_cast<float>(m_cellSize));
    vertReferenceLines[3].setX(vertReferenceLines[3].x() - static_cast<float>(m_cellSize));

    /*----------------------------grid----------------------------*/
    ctx->glLineWidth(1.0f);
    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(0.0f, 0.0f, 0.0f, 200.0f)));
    shaderProgram->setAttributeArray(posLoc, grid.constData());
    ctx->glDrawArrays(GL_LINES, 0, grid.size());
    ctx->glLineWidth(1.0f);

    /*----------------------------dimentions plane----------------------------*/
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ctx->glLineWidth(4.0f);
    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(0.0f, 200.0f, 255.0f, 200.0f)));
    shaderProgram->setAttributeArray(posLoc, sceneBoundsPlane.constData());
    // ctx->glDrawArrays(GL_QUADS, 0, sceneBoundsPlane.size());
    ctx->glLineWidth(1.0f);
    glDisable(GL_BLEND);

    /*----------------------------dimentions lines----------------------------*/
    glEnable(2852);
    
    ctx->glLineWidth(4.0f);
    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(0.0f, 104.0f, 145.0f, 0.0f)));
    shaderProgram->setAttributeArray(posLoc, horzSizeLine.constData());
    ctx->glDrawArrays(GL_LINES, 0, horzSizeLine.size());
    shaderProgram->setAttributeArray(posLoc, vertSizeLine.constData());
    ctx->glDrawArrays(GL_LINES, 0, vertSizeLine.size());

    glDisable(2852);

    /*----------------------------dimentions lines reference----------------------------*/
    shaderProgram->setAttributeArray(posLoc, horzReferenceLines.constData());
    ctx->glDrawArrays(GL_LINES, 0, horzReferenceLines.size());
    shaderProgram->setAttributeArray(posLoc, vertReferenceLines.constData());
    ctx->glDrawArrays(GL_LINES, 0, vertReferenceLines.size());

    ctx->glLineWidth(1.0f);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}

void PlaneGrid::PlaneGridRenderImplementation::render(QOpenGLFunctions *ctx,
                                                      const QMatrix4x4 &model,
                                                      const QMatrix4x4 &view,
                                                      const QMatrix4x4 &projection,
                                                      const QMap<QString,std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
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

    shaderProgram->setUniformValue(matrixLoc, projection * view * model);
    shaderProgram->enableAttributeArray(posLoc);

    QVector<QVector3D> grid;

    int cellCount = std::round(std::max(m_size.height(), m_size.width()) * 1.3 / m_cellSize);

    if(cellCount % 2 != 0)
        cellCount++;

    int gridSize = cellCount * m_cellSize;

    for(int i = 0; i <= cellCount; i++){
        grid.append({
            {m_position.x() - static_cast<float>(gridSize) / 2.0f, m_position.y() - static_cast<float>(gridSize) / 2.0f + static_cast<float>(i) * m_cellSize, m_position.z()},
            {m_position.x() + static_cast<float>(gridSize) / 2.0f, m_position.y() - static_cast<float>(gridSize) / 2.0f + static_cast<float>(i) * m_cellSize, m_position.z()},
            {m_position.x() - static_cast<float>(gridSize) / 2.0f + static_cast<float>(i) * m_cellSize,m_position.y() - static_cast<float>(gridSize) / 2.0f, m_position.z()},
            {m_position.x() - static_cast<float>(gridSize) / 2.0f + static_cast<float>(i) * m_cellSize,m_position.y() + static_cast<float>(gridSize) / 2.0f, m_position.z()}
        });
    }

    QVector<QVector3D> sceneBoundsPlane{
                                        {m_position.x()-static_cast<float>(m_size.height()/2.0f), m_position.y()-static_cast<float>(m_size.width()/2.0f), m_position.z()},
                                        {m_position.x()+static_cast<float>(m_size.height()/2.0f), m_position.y()-static_cast<float>(m_size.width()/2.0f), m_position.z()},
                                        {m_position.x()+static_cast<float>(m_size.height()/2.0f), m_position.y()+static_cast<float>(m_size.width()/2.0f), m_position.z()},
                                        {m_position.x()-static_cast<float>(m_size.height()/2.0f), m_position.y()+static_cast<float>(m_size.width()/2.0f), m_position.z()},
                                        };

    QVector<QVector3D> horzSizeLine{
        {m_position.x()-static_cast<float>(m_size.height()/2.0f), m_position.y()-static_cast<float>(m_size.width()/2.0f) - m_cellSize, m_position.z()},
        {m_position.x()+static_cast<float>(m_size.height()/2.0f), m_position.y()-static_cast<float>(m_size.width()/2.0f) - m_cellSize, m_position.z()}
    };

    QVector<QVector3D> vertSizeLine{
                                    {m_position.x()-static_cast<float>(m_size.height()/2.0f) - m_cellSize, m_position.y()-static_cast<float>(m_size.width()/2.0f), m_position.z()},
                                    {m_position.x()-static_cast<float>(m_size.height()/2.0f) - m_cellSize, m_position.y()+static_cast<float>(m_size.width()/2.0f), m_position.z()},
                                    };

    QVector<QVector3D> horzReferenceLines{
        sceneBoundsPlane.at(0),
        horzSizeLine.at(0),
        sceneBoundsPlane.at(1),
        horzSizeLine.at(1)
    };
    horzReferenceLines[1].setY(horzReferenceLines[1].y() - static_cast<float>(m_cellSize));
    horzReferenceLines[3].setY(horzReferenceLines[3].y() - static_cast<float>(m_cellSize));

    QVector<QVector3D> vertReferenceLines{
        sceneBoundsPlane.at(1),
        vertSizeLine.at(0),
        sceneBoundsPlane.at(2),
        vertSizeLine.at(1)
    };
    vertReferenceLines[1].setX(vertReferenceLines[1].x() - static_cast<float>(m_cellSize));
    vertReferenceLines[3].setX(vertReferenceLines[3].x() - static_cast<float>(m_cellSize));

    /*----------------------------grid----------------------------*/
    ctx->glLineWidth(1.0f);
    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(100.0f, 100.0f, 100.0f, 200.0f)));
    shaderProgram->setAttributeArray(posLoc, grid.constData());
    ctx->glDrawArrays(GL_LINES, 0, grid.size());
    ctx->glLineWidth(1.0f);

    /*----------------------------dimentions plane----------------------------*/
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ctx->glLineWidth(4.0f);
    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(0.0f, 200.0f, 255.0f, 200.0f)));
    shaderProgram->setAttributeArray(posLoc, sceneBoundsPlane.constData());
    // ctx->glDrawArrays(GL_QUADS, 0, sceneBoundsPlane.size());
    ctx->glLineWidth(1.0f);
    glDisable(GL_BLEND);

    /*----------------------------dimentions lines----------------------------*/
    glEnable(2852);
    
    ctx->glLineWidth(4.0f);
    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(0.0f, 104.0f, 145.0f, 0.0f)));
    shaderProgram->setAttributeArray(posLoc, horzSizeLine.constData());
    ctx->glDrawArrays(GL_LINES, 0, horzSizeLine.size());
    shaderProgram->setAttributeArray(posLoc, vertSizeLine.constData());
    ctx->glDrawArrays(GL_LINES, 0, vertSizeLine.size());

    glDisable(2852);

    // TODO
    /*----------------------------dimentions lines reference----------------------------*/
    /*shaderProgram->setAttributeArray(posLoc, horzReferenceLines.constData());
    ctx->glDrawArrays(GL_LINES, 0, horzReferenceLines.size());
    shaderProgram->setAttributeArray(posLoc, vertReferenceLines.constData());
    ctx->glDrawArrays(GL_LINES, 0, vertReferenceLines.size());

    ctx->glLineWidth(1.0f);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();

    // Rendering text
    QRectF vport = DrawUtils::viewportRect(ctx);

    QVector3D p1 = horzSizeLine.at(0), p2 = horzSizeLine.at(1);
    QVector3D p = p1 + (p2 - p1).normalized() * (p1.distanceToPoint(p2) / 2.0f);
    QVector2D p_screen = p.project(view * model, projection, vport.toRect()).toVector2D();
    p_screen.setY(vport.height() - p_screen.y());

    QMatrix4x4 textProjection;
    textProjection.ortho(vport.toRect());

    TextRenderer::instance().render(QString("%1 m").arg(p1.distanceToPoint(p2)),
                                    0.4f,
                                    p_screen,
                                    ctx,
                                    textProjection
                                    );

    p1 = vertSizeLine.at(0);
    p2 = vertSizeLine.at(1);
    p = p1 + (p2 - p1).normalized() * (p1.distanceToPoint(p2) / 2.0f);
    p_screen = p.project(view * model, projection, vport.toRect()).toVector2D();
    p_screen.setY(vport.height() - p_screen.y());

    TextRenderer::instance().render(QString("%1 m").arg(p1.distanceToPoint(p2)),
                                    0.4f,
                                    p_screen,
                                    ctx,
                                    textProjection
                                    );*/
}

