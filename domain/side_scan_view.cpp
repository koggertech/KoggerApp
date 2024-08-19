#include "side_scan_view.h"

#include <QtMath>


SideScanView::SideScanView(QObject* parent) :
    SceneObject(new SideScanViewRenderImplementation, parent)
{

}

SideScanView::~SideScanView()
{

}

SideScanView::SideScanViewRenderImplementation::SideScanViewRenderImplementation()
{

}

void SideScanView::SideScanViewRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible) {
        return;
    }

    auto shaderProgram = shaderProgramMap.value("static", nullptr);

    if (!shaderProgram) {
        qWarning() << "Shader program 'static' not found!";
        return;
    }

    shaderProgram->bind();
    shaderProgram->setUniformValue("mvp", mvp);

    int posLoc = shaderProgram->attributeLocation("position");
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, m_data.constData());

    shaderProgram->setUniformValue("color", QVector4D(0.0f, 1.0f, 0.0f, 1.0f));
    ctx->glDrawElements(GL_LINES, evenIndices_.size(), GL_UNSIGNED_INT, evenIndices_.constData());

    shaderProgram->setUniformValue("color", QVector4D(1.0f, 0.0f, 0.0f, 1.0f));
    ctx->glDrawElements(GL_LINES, oddIndices_.size(), GL_UNSIGNED_INT, oddIndices_.constData());

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}

void SideScanView::updateData()
{
    if (!datasetPtr_) {
        qDebug() << "dataset is nullptr!";
        return;
    }

    isOdd_.clear();
    usedIndx_.clear();

    auto renderImpl = RENDER_IMPL(SideScanView);

    QVector<QVector3D> vertices;
    QVector<int> evenIndices;
    QVector<int> oddIndices;

    if (auto chList = datasetPtr_->channelsList(); chList.size() == 2) {
        auto it = chList.begin();
        int fCh{ it.key() }, sCh { (++it).key() };
        uint64_t currIndx = 0;

        for (int i = 0; i < datasetPtr_->size(); ++i) {
            auto epoch = datasetPtr_->fromIndex(i);

            if (!epoch) {
                continue;
            }

            auto pos = epoch->getPositionGNSS().ned;
            pos.d = 0.0f;
            auto yaw = epoch->yaw();

            auto fChartsPrt = epoch->chart(fCh);
            auto sChartsPrt = epoch->chart(sCh);

            if (isfinite(pos.n) && isfinite(pos.e) && isfinite(yaw)) {
                double azimuthRad = qDegreesToRadians(yaw);

                if (fChartsPrt) {
                    double leftAzimuthRad = azimuthRad - M_PI_2;

                    float lDistance = fChartsPrt->range();
                    float lX = pos.n + lDistance * qCos(leftAzimuthRad);
                    float lY = pos.e + lDistance * qSin(leftAzimuthRad);
                    float lZ = pos.d;

                    vertices.append(QVector3D(lX, lY, lZ));
                    evenIndices.append(currIndx++);
                    vertices.append(QVector3D(pos.n, pos.e, pos.d));
                    evenIndices.append(currIndx++);

                    isOdd_.append(false);
                    usedIndx_.append(i);
                }

                if (sChartsPrt) {
                    double rightAzimuthRad = azimuthRad + M_PI_2;

                    float rDistance = sChartsPrt->range();
                    float rX = pos.n + rDistance * qCos(rightAzimuthRad);
                    float rY = pos.e + rDistance * qSin(rightAzimuthRad);
                    float rZ = pos.d;

                    vertices.append(QVector3D(pos.n, pos.e, pos.d));
                    oddIndices.append(currIndx++);
                    vertices.append(QVector3D(rX, rY, rZ));
                    oddIndices.append(currIndx++);

                    isOdd_.append(true);
                    usedIndx_.append(i);
                }
            }
        }
    }

    renderImpl->m_data = vertices;
    renderImpl->evenIndices_ = evenIndices;
    renderImpl->oddIndices_ = oddIndices;

    updatePixelMatrix(vertices, matrixScaleFactor_, lineThickness_);

    QImage image = pixelMatrixToImage(pixelMatrix_);
    QString imagePath = "C:/Users/salty/Desktop/textures/bres.png";

    if (!image.save(imagePath)) {
        qWarning() << "Failed to save image at" << imagePath;
    }
    else {
        qDebug() << "Image saved successfully at" << imagePath;
    }


    Q_EMIT changed();
}

void SideScanView::clear()
{
    auto renderImpl = RENDER_IMPL(SideScanView);

    renderImpl->evenIndices_.clear();
    renderImpl->oddIndices_.clear();
    renderImpl->m_data.clear();

    Q_EMIT changed();
}

void SideScanView::setDatasetPtr(Dataset* datasetPtr)
{
    datasetPtr_ = datasetPtr;
}

void SideScanView::updatePixelMatrix(const QVector<QVector3D> &vertices, float scaleFactor, int lineThickness)
{
    if (vertices.isEmpty()) {
        return;
    }

    pixelMatrix_.clear();

    auto [minX, maxX] = std::minmax_element(vertices.begin(), vertices.end(), [](const QVector3D &a, const QVector3D &b) { return a.x() < b.x(); });
    auto [minY, maxY] = std::minmax_element(vertices.begin(), vertices.end(), [](const QVector3D &a, const QVector3D &b) { return a.y() < b.y(); });

    int width = static_cast<int>(std::ceil((maxX->x() - minX->x()) * scaleFactor)) + 1;
    int height = static_cast<int>(std::ceil((maxY->y() - minY->y()) * scaleFactor)) + 1;

    pixelMatrix_ = QVector<QVector<Point>>(height, QVector<Point>(width));

    for (int i = 0; i < vertices.size(); i += 2) {
        if (i + 1 >= vertices.size()) {
            break;
        }

        auto epoch = datasetPtr_->fromIndex(usedIndx_[i / 2]);
        bool isOdd = isOdd_[i / 2];

        auto chList = datasetPtr_->channelsList();
        auto it = chList.begin();
        int fCh{ it.key() }, sCh{ (++it).key() };
        auto charts = epoch->chart(isOdd ? sCh : fCh);

        if (!charts) {
            continue;
        }

        uint64_t chartsSize = charts->amplitude.size();

        QVector3D start = vertices[i];
        QVector3D end = vertices[i + 1];

        // from, to pixel points
        int x1 = static_cast<int>(std::round((start.x() - minX->x()) * scaleFactor));
        int y1 = static_cast<int>(std::round((start.y() - minY->y()) * scaleFactor));
        int x2 = static_cast<int>(std::round((end.x() - minX->x()) * scaleFactor));
        int y2 = static_cast<int>(std::round((end.y() - minY->y()) * scaleFactor));

        float totalDistance = std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2));

        // Bresenham's algorhitm
        int dx = std::abs(x2 - x1);
        int dy = std::abs(y2 - y1);
        int sx = (x1 < x2) ? 1 : -1;
        int sy = (y1 < y2) ? 1 : -1;
        int err = dx - dy;

        while (true) { // line passing
            float currentDistance = std::sqrt(std::pow(x1 - static_cast<int>(std::round((start.x() - minX->x()) * scaleFactor)), 2) +
                                              std::pow(y1 - static_cast<int>(std::round((start.y() - minY->y()) * scaleFactor)), 2));
            float progress = currentDistance / totalDistance;

            // pixel point
            QVector3D pixelPos( start.x() + progress * (end.x() - start.x()),
                                start.y() + progress * (end.y() - start.y()),
                                -1.f * static_cast<float>(epoch->distProccesing(isOdd ? fCh : sCh)));

            // boat point
            QVector3D boatPos(  epoch->getPositionGNSS().ned.n,
                                epoch->getPositionGNSS().ned.e,
                                0.0f);

            float beamDist = pixelPos.distanceToPoint(boatPos);
            uint64_t ampIndx = static_cast<uint64_t>(std::floor(beamDist * 100.0f));

            // coloring
            for (int dx = -lineThickness; dx <= lineThickness; ++dx) {
                for (int dy = -lineThickness; dy <= lineThickness; ++dy) {
                    int nx = x1 + dx;
                    int ny = y1 + dy;
                    if (nx >= 0 && ny >= 0 && nx < width && ny < height) {

                        if ((!isOdd && progress > 0.995f) || (isOdd && progress < 0.005f)) { // red line
                            pixelMatrix_[ny][nx].color = QColor(255, 0, 0);
                        }
                        else if (chartsSize > ampIndx) {
                            int cVal = charts->amplitude[ampIndx] * (1.5 + ampIndx * 0.0002f);
                            cVal = std::min(255, cVal);
                            pixelMatrix_[ny][nx].color = QColor(cVal, cVal, cVal);
                        }
                    }
                }
            }

            if (x1 == x2 && y1 == y2)
                break;

            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x1 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y1 += sy;
            }
        }
    }
}

QImage SideScanView::pixelMatrixToImage(const QVector<QVector<Point>> &pixelMatrix)
{
    if (pixelMatrix.isEmpty() || pixelMatrix[0].isEmpty()) {
        return QImage();
    }

    int width = pixelMatrix[0].size();
    int height = pixelMatrix.size();

    QImage image(width, height, QImage::Format_RGB32);
    image.fill(Qt::white);
    QColor white(0, 0, 0, 255);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {

            if (pixelMatrix[y][x].color != QColor()) {
                image.setPixel(x, y, pixelMatrix[y][x].color.rgb());
            }
            else {
                image.setPixel(x, y, white.rgb());
            }
        }
    }

    QTransform transform;
    transform.rotate(-90.0f);

    return image.transformed(transform);
}
