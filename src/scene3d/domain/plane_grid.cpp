#include "plane_grid.h"

#include <QtMath>
#include <utility>
#include "draw_utils.h"
#include <text_renderer.h>


PlaneGrid::PlaneGrid(QObject *parent)
    : SceneObject(new PlaneGridRenderImplementation, parent)
{}

void PlaneGrid::setPlane(const Plane &plane)
{
    auto r = RENDER_IMPL(PlaneGrid);

    r->m_size = plane.size();
    r->m_position = { plane.center().x(), plane.center().y(), 0.0f };

    Q_EMIT changed();
}

void PlaneGrid::setSize(const QSizeF &size)
{
    RENDER_IMPL(PlaneGrid)->m_size = size;

    Q_EMIT changed();
}

void PlaneGrid::setPosition(const QVector3D &pos)
{
    m_position = { pos.x(), pos.y(), 0.0f };
    RENDER_IMPL(PlaneGrid)->m_position = m_position;

    Q_EMIT changed();
}

void PlaneGrid::setCellSize(int size)
{
    m_cellSize = size;
    RENDER_IMPL(PlaneGrid)->m_cellSize = size;

    Q_EMIT changed();
}

void PlaneGrid::setType(bool type)
{
    RENDER_IMPL(PlaneGrid)->defType_ = type;
}

void PlaneGrid::setCircleSize(int val)
{
    RENDER_IMPL(PlaneGrid)->circleSize_ = val;

    Q_EMIT changed();
}

void PlaneGrid::setCircleStep(int val)
{
    RENDER_IMPL(PlaneGrid)->circleStep_ = val;

    Q_EMIT changed();
}

void PlaneGrid::setCircleAngle(int val)
{
    RENDER_IMPL(PlaneGrid)->circleAngle_ = val;

    Q_EMIT changed();
}

void PlaneGrid::setCircleLabels(bool state)
{
    RENDER_IMPL(PlaneGrid)->circleLabels_ = state;

    Q_EMIT changed();
}

void PlaneGrid::setCirclePosition(const QVector3D &pos)
{
    RENDER_IMPL(PlaneGrid)->circlePosition_ = pos;

    Q_EMIT changed();
}

void PlaneGrid::setActiveZeroing(bool state)
{
    RENDER_IMPL(PlaneGrid)->isActiveZeroing_ = state;

    Q_EMIT changed();
}

void PlaneGrid::clear()
{
    m_size = {10,10};
    m_cellSize = 1.0f;
    m_position = {0.0f, 0.0f, 0.0f};

    auto* r = RENDER_IMPL(PlaneGrid);
    r->m_size = {10,10};
    r->m_cellSize = 1.0f;
    r->m_position = {0.0f, 0.0f, 0.0f};
    r->circlePosition_ = QVector3D();
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

// void PlaneGrid::PlaneGridRenderImplementation::render(QOpenGLFunctions *ctx,
//                                                       const QMatrix4x4 &mvp,
//                                                       const QMap<QString,
//                                                       std::shared_ptr<QOpenGLShaderProgram> > &shaderProgramMap) const
// {
//     if (!defType_) {
//         return;
//     }

//     if(!shaderProgramMap.contains("static"))
//         return;

//     auto shaderProgram = shaderProgramMap["static"];

//     if (!shaderProgram->bind()){
//         qCritical() << "Error binding shader program.";
//         return;
//     }

//     int posLoc    = shaderProgram->attributeLocation("position");
//     int colorLoc  = shaderProgram->uniformLocation("color");
//     int matrixLoc = shaderProgram->uniformLocation("matrix");

//     shaderProgram->setUniformValue(matrixLoc, mvp);
//     shaderProgram->enableAttributeArray(posLoc);

//     QVector<QVector3D> grid;

//     int cellCount = std::round(std::max(m_size.height(), m_size.width()) * 1.3 / m_cellSize);

//     if(cellCount % 2 != 0)
//         cellCount++;

//     int gridSize = cellCount * m_cellSize;

//     for(int i = 0; i <= cellCount; i++){
//         grid.append({
//                 {m_position.x() - static_cast<float>(gridSize) / 2.0f, m_position.y() - static_cast<float>(gridSize) / 2.0f + static_cast<float>(i) * m_cellSize, m_position.z()},
//                 {m_position.x() + static_cast<float>(gridSize) / 2.0f, m_position.y() - static_cast<float>(gridSize) / 2.0f + static_cast<float>(i) * m_cellSize, m_position.z()},
//                 {m_position.x() - static_cast<float>(gridSize) / 2.0f + static_cast<float>(i) * m_cellSize,m_position.y() - static_cast<float>(gridSize) / 2.0f, m_position.z()},
//                 {m_position.x() - static_cast<float>(gridSize) / 2.0f + static_cast<float>(i) * m_cellSize,m_position.y() + static_cast<float>(gridSize) / 2.0f, m_position.z()}
//             });
//     }

//     QVector<QVector3D> sceneBoundsPlane{
//         {m_position.x()-static_cast<float>(m_size.height()/2.0f), m_position.y()-static_cast<float>(m_size.width()/2.0f), m_position.z()},
//         {m_position.x()+static_cast<float>(m_size.height()/2.0f), m_position.y()-static_cast<float>(m_size.width()/2.0f), m_position.z()},
//         {m_position.x()+static_cast<float>(m_size.height()/2.0f), m_position.y()+static_cast<float>(m_size.width()/2.0f), m_position.z()},
//         {m_position.x()-static_cast<float>(m_size.height()/2.0f), m_position.y()+static_cast<float>(m_size.width()/2.0f), m_position.z()},
//     };

//     QVector<QVector3D> horzSizeLine{
//         {m_position.x()-static_cast<float>(m_size.height()/2.0f), m_position.y()-static_cast<float>(m_size.width()/2.0f) - m_cellSize, m_position.z()},
//         {m_position.x()+static_cast<float>(m_size.height()/2.0f), m_position.y()-static_cast<float>(m_size.width()/2.0f) - m_cellSize, m_position.z()}
//     };

//     QVector<QVector3D> vertSizeLine{
//         {m_position.x()-static_cast<float>(m_size.height()/2.0f) - m_cellSize, m_position.y()-static_cast<float>(m_size.width()/2.0f), m_position.z()},
//         {m_position.x()-static_cast<float>(m_size.height()/2.0f) - m_cellSize, m_position.y()+static_cast<float>(m_size.width()/2.0f), m_position.z()},
//     };

//     QVector<QVector3D> horzReferenceLines{
//         sceneBoundsPlane.at(0),
//         horzSizeLine.at(0),
//         sceneBoundsPlane.at(1),
//         horzSizeLine.at(1)
//     };
//     horzReferenceLines[1].setY(horzReferenceLines[1].y() - static_cast<float>(m_cellSize));
//     horzReferenceLines[3].setY(horzReferenceLines[3].y() - static_cast<float>(m_cellSize));

//     QVector<QVector3D> vertReferenceLines{
//         sceneBoundsPlane.at(1),
//         vertSizeLine.at(0),
//         sceneBoundsPlane.at(2),
//         vertSizeLine.at(1)
//     };
//     vertReferenceLines[1].setX(vertReferenceLines[1].x() - static_cast<float>(m_cellSize));
//     vertReferenceLines[3].setX(vertReferenceLines[3].x() - static_cast<float>(m_cellSize));

//     /*----------------------------grid----------------------------*/
//     ctx->glLineWidth(1.0f);
//     shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(0.0f, 0.0f, 0.0f, 200.0f)));
//     shaderProgram->setAttributeArray(posLoc, grid.constData());
//     ctx->glDrawArrays(GL_LINES, 0, grid.size());
//     ctx->glLineWidth(1.0f);

//     /*----------------------------dimentions plane----------------------------*/
//     ctx->glEnable(GL_BLEND);
//     ctx->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//     ctx->glLineWidth(1.0f);
//     shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(0.0f, 200.0f, 255.0f, 200.0f)));
//     shaderProgram->setAttributeArray(posLoc, sceneBoundsPlane.constData());
//     ctx->glDrawArrays(GL_LINE_LOOP, 0, sceneBoundsPlane.size());
//     ctx->glDisable(GL_BLEND);

//     /*----------------------------dimentions lines----------------------------*/
//     shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(0.0f, 104.0f, 145.0f, 255.0f)));
//     shaderProgram->setAttributeArray(posLoc, horzSizeLine.constData());
//     ctx->glDrawArrays(GL_LINES, 0, horzSizeLine.size());
//     shaderProgram->setAttributeArray(posLoc, vertSizeLine.constData());
//     ctx->glDrawArrays(GL_LINES, 0, vertSizeLine.size());

//     /*----------------------------dimentions lines reference----------------------------*/
//     shaderProgram->setAttributeArray(posLoc, horzReferenceLines.constData());
//     ctx->glDrawArrays(GL_LINES, 0, horzReferenceLines.size());
//     shaderProgram->setAttributeArray(posLoc, vertReferenceLines.constData());
//     ctx->glDrawArrays(GL_LINES, 0, vertReferenceLines.size());

//     ctx->glLineWidth(1.0f);

//     shaderProgram->disableAttributeArray(posLoc);
//     shaderProgram->release();
// }

void PlaneGrid::PlaneGridRenderImplementation::render(QOpenGLFunctions *ctx,
                                                      const QMatrix4x4 &model,
                                                      const QMatrix4x4 &view,
                                                      const QMatrix4x4 &projection,
                                                      const QMap<QString,std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (defType_) {
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

        ctx->glLineWidth(1.5f);

        /*----------------------------grid----------------------------*/
        shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(100.0f, 100.0f, 100.0f, 200.0f)));
        shaderProgram->setAttributeArray(posLoc, grid.constData());
        ctx->glDrawArrays(GL_LINES, 0, grid.size());

        /*----------------------------dimentions plane----------------------------*/
        ctx->glEnable(GL_BLEND);
        ctx->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(0.0f, 200.0f, 255.0f, 200.0f)));
        shaderProgram->setAttributeArray(posLoc, sceneBoundsPlane.constData());
        ctx->glDrawArrays(GL_LINE_LOOP, 0, sceneBoundsPlane.size());
        ctx->glDisable(GL_BLEND);

        /*----------------------------dimentions lines----------------------------*/
        shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(QColor(0.0f, 104.0f, 145.0f, 255.0f)));
        shaderProgram->setAttributeArray(posLoc, horzSizeLine.constData());
        ctx->glDrawArrays(GL_LINES, 0, horzSizeLine.size());
        shaderProgram->setAttributeArray(posLoc, vertSizeLine.constData());
        ctx->glDrawArrays(GL_LINES, 0, vertSizeLine.size());

        ctx->glLineWidth(1.0f);

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

        shaderProgram->disableAttributeArray(posLoc);
        shaderProgram->release();
    }
    else { // circle render
        if (!isActiveZeroing_ && circlePosition_.isNull()) {
            return;
        }

        if (!shaderProgramMap.contains("static")) {
            return;
        }

        auto shaderProgram = shaderProgramMap["static"];
        if (!shaderProgram->bind()) {
            qCritical() << "Error binding shader program.";
            return;
        }

        int posLoc    = shaderProgram->attributeLocation("position");
        int colorLoc  = shaderProgram->uniformLocation("color");
        int matrixLoc = shaderProgram->uniformLocation("matrix");

        shaderProgram->setUniformValue(matrixLoc, projection * view * model);
        shaderProgram->enableAttributeArray(posLoc);

        const QVector3D center = circlePosition_;

        const int   sizeIdx   = qBound(1, circleSize_, 3);
        const float maxRadius = 50.0f * static_cast<float>(sizeIdx);

        const int   stepIdx   = qBound(1, circleStep_, 20);
        const float radiusStep = static_cast<float>(stepIdx);

        const int circleSegments = 128;

        ctx->glLineWidth(1.5f);

        shaderProgram->setUniformValue(
            colorLoc,
            DrawUtils::colorToVector4d(QColor(255, 255, 255, 255))
            );

        QVector<QVector3D> circleVertices;
        circleVertices.reserve(circleSegments + 1);

        const int maxRing = static_cast<int>(std::floor(maxRadius / radiusStep)); // сколько окружностей

        // circles
        for (int ring = 1; ring <= maxRing; ++ring) {
            const float r = ring * radiusStep;

            circleVertices.clear();

            for (int i = 0; i <= circleSegments; ++i) {
                const float t = static_cast<float>(i) / static_cast<float>(circleSegments);
                const float angle = t * static_cast<float>(2.0 * M_PI);

                const float x = center.x() + qCos(angle) * r;
                const float y = center.y() + qSin(angle) * r;

                circleVertices.append(QVector3D(x, y, center.z()));
            }

            shaderProgram->setAttributeArray(posLoc, circleVertices.constData());
            ctx->glDrawArrays(GL_LINE_STRIP, 0, circleVertices.size());
        }

        ctx->glLineWidth(2.0f);

        int parts = 0;
        switch (circleAngle_) {
        case 1: parts = 2;  break;
        case 2: parts = 4;  break;
        case 3: parts = 6;  break;
        case 4: parts = 12;  break;
        case 5: parts = 24;  break;
        default: parts = 4; break;
        }

        const float startDeg = 0.0f;
        const float stepDeg  = 360.0f / static_cast<float>(parts);

        QVector<QVector3D> rayVertices;
        rayVertices.reserve(parts * 2);

        for (int i = 0; i < parts; ++i) {
            const float angleDeg = startDeg + stepDeg * static_cast<float>(i);
            const float angleRad = qDegreesToRadians(angleDeg);

            const QVector3D p0 = center;
            const QVector3D p1(center.x() + qCos(angleRad) * maxRadius,
                               center.y() + qSin(angleRad) * maxRadius,
                               center.z());

            rayVertices.append(p0);
            rayVertices.append(p1);
        }

        shaderProgram->setUniformValue(
            colorLoc,
            DrawUtils::colorToVector4d(QColor(200, 200, 200, 255))
            );
        shaderProgram->setAttributeArray(posLoc, rayVertices.constData());
        ctx->glDrawArrays(GL_LINES, 0, rayVertices.size());

        shaderProgram->disableAttributeArray(posLoc);
        shaderProgram->release();
        ctx->glLineWidth(1.0f);

        // text
        if (circleLabels_) {
            QRectF vport = DrawUtils::viewportRect(ctx);

            QMatrix4x4 textProjection;
            textProjection.ortho(vport.toRect());

            const QMatrix4x4 modelView = view * model;
            QVector<TextRenderer::Text2DItem> labelItems;
            labelItems.reserve(maxRing + parts);

            const float mainAngleDeg = -90.0f;
            const float mainAngleRad = qDegreesToRadians(mainAngleDeg);

            for (int ring = 1; ring <= maxRing; ++ring) {
                const float r = ring * radiusStep;

                QVector3D pWorld(center.x() + qCos(mainAngleRad) * r,
                                 center.y() + qSin(mainAngleRad) * r,
                                 center.z());

                QVector3D pWin = pWorld.project(modelView, projection, vport.toRect());
                QVector2D pScreen(pWin.x(), vport.height() - pWin.y());
                pScreen.setY(pScreen.y() + 8.0f);

                QString text = QString::number(static_cast<int>(qRound(r))) + " m";
                labelItems.append(TextRenderer::Text2DItem{std::move(text), 0.7f, pScreen, true});
            }

            const float labelRadius = maxRadius + radiusStep * 0.7f;

            for (int i = 0; i < parts; ++i) {
                const float angleDeg = startDeg + stepDeg * static_cast<float>(i);
                const float angleRad = qDegreesToRadians(angleDeg);

                QVector3D pWorld(center.x() + qCos(angleRad) * labelRadius,
                                 center.y() + qSin(angleRad) * labelRadius,
                                 center.z());

                QVector3D pWin = pWorld.project(modelView, projection, vport.toRect());
                QVector2D pScreen(pWin.x(), vport.height() - pWin.y());

                pScreen.setY(pScreen.y() + 4.0f);

                QString text = QString::number(static_cast<int>(qRound(angleDeg))) + QChar(0x00B0);
                labelItems.append(TextRenderer::Text2DItem{std::move(text), 0.8f, pScreen, true});
            }

            if (!labelItems.isEmpty()) {
                TextRenderer::instance().render2DBatch(labelItems, ctx, textProjection, shaderProgramMap);
            }
        }
    }
}
