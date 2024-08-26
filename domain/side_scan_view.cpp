#include "side_scan_view.h"

#include <QtMath>


SideScanView::SideScanView(QObject* parent) :
    SceneObject(new SideScanViewRenderImplementation, parent),
    datasetPtr_(nullptr),
    scaleFactor_(10.0f),
    segFChannelId_(-1),
    segSChannelId_(-1)
{
    updateColorTable();
}

SideScanView::~SideScanView()
{

}

void SideScanView::updateData()
{
    if (!datasetPtr_) {
        qDebug() << "dataset is nullptr!";
        return;
    }

    updateChannelsIds();
    clear();

    QVector<char> isOdds;
    QVector<int> epochIndxs;
    auto epochCount = datasetPtr_->size();
    auto renderImpl = RENDER_IMPL(SideScanView);

    renderImpl->m_data.reserve(epochCount * 4);
    renderImpl->evenIndices_.reserve(epochCount * 2);
    renderImpl->oddIndices_.reserve(epochCount * 2);
    isOdds.reserve(epochCount * 2);
    epochIndxs.reserve(epochCount * 2);
    uint64_t currIndx = 0;

    for (int i = 0; i < epochCount; ++i) {
        auto epoch = datasetPtr_->fromIndex(i);
        if (!epoch) {
            continue;
        }

        auto pos = epoch->getPositionGNSS().ned;
        auto yaw = epoch->yaw();

        if (isfinite(pos.n) && isfinite(pos.e) && isfinite(yaw)) {
            double azRad = qDegreesToRadians(yaw);
            if (auto segFCharts = epoch->chart(segFChannelId_); segFCharts) {
                double leftAzRad = azRad - M_PI_2;
                float lDist = segFCharts->range();
                renderImpl->m_data.append(QVector3D(pos.n + lDist * qCos(leftAzRad), pos.e + lDist * qSin(leftAzRad),  0.0f));
                renderImpl->m_data.append(QVector3D(pos.n, pos.e, 0.0f));
                renderImpl->evenIndices_.append(currIndx++);
                renderImpl->evenIndices_.append(currIndx++);
                isOdds.append('0');
                epochIndxs.append(i);
            }

            if (auto segSCharts = epoch->chart(segSChannelId_); segSCharts) {
                double rightAzRad = azRad + M_PI_2;
                float rDist = segSCharts ->range();
                renderImpl->m_data.append(QVector3D(pos.n, pos.e, 0.0f));
                renderImpl->m_data.append(QVector3D(pos.n + rDist * qCos(rightAzRad), pos.e + rDist * qSin(rightAzRad), 0.0f));
                renderImpl->oddIndices_.append(currIndx++);
                renderImpl->oddIndices_.append(currIndx++);
                isOdds.append('1');
                epochIndxs.append(i);
            }
        }
    }

    Q_EMIT changed();


    // processing
    matParams_ = getMatrixParams(renderImpl->m_data);
    // height matrix
    //
    // texture
    updateColorMatrix(renderImpl->m_data, isOdds, epochIndxs);
    QString path = "C:/Users/salty/Desktop/textures/bres.png";
    saveImageToFile(image_, path);
    image_ = QImage();
}

void SideScanView::clear()
{
    auto renderImpl = RENDER_IMPL(SideScanView);
    renderImpl->m_data.clear();
    renderImpl->evenIndices_.clear();
    renderImpl->oddIndices_.clear();

    image_ = QImage();
    matParams_ = MatrixParams();

    Q_EMIT changed();
}

void SideScanView::setScaleFactor(float scaleFactor)
{
    scaleFactor_ = scaleFactor;
}

void SideScanView::setDatasetPtr(Dataset* datasetPtr)
{
    datasetPtr_ = datasetPtr;
}

void SideScanView::updateColorTable()
{
    int numColors = 256;
    colorTable_.resize(numColors);
    for (int i = 0; i < numColors; ++i) {
        int grayValue = i * 255 / (numColors - 1);
        colorTable_[i] = qRgb(grayValue, grayValue, grayValue);
    }
}

void SideScanView::updateChannelsIds()
{
    segFChannelId_ = -1;
    segSChannelId_ = -1;

    if (datasetPtr_) {
        if (auto chList = datasetPtr_->channelsList(); chList.size() == 2) {
            auto it = chList.begin();
            segFChannelId_ = it.key();
            segSChannelId_ = (++it).key();
        }
    }
}

void SideScanView::updateColorMatrix(const QVector<QVector3D> &vertices, QVector<char>& isOdds,
                                     QVector<int> epochIndxs, int interpLineWidth, bool sideScanLineDrawing)
{
    if (vertices.isEmpty()) {
        return;
    }

    if (!matParams_.isValid()) {
        return;
    }

    image_ = QImage(matParams_.width_, matParams_.height_,  QImage::Format_Indexed8);
    uchar* imageData = image_.bits();
    int bytesPerLine = image_.bytesPerLine();
    int bytesInPix = bytesPerLine / image_.width();

    // interpolation only to the next epoch, when the segment side matches
    for (int i = 0; i < vertices.size(); i += 2) { // step for 1 line
        if (i + 8 >= vertices.size()) {
            break;
        }
        int segFBegVertIndx = i;
        int segFEndVertIndx = i + 1;
        int segSBegVertIndx = i + 2;
        int segSEndVertIndx = i + 3;
        int segFIndx = i / 2;
        int segSIndx = (i + 2) / 2;
        // going to next epoch if needed
        if (epochIndxs[segFIndx] == epochIndxs[segSIndx] ||
            isOdds[segFIndx] != isOdds[segSIndx]) {
            ++segSIndx;
            segSBegVertIndx += 2;
            segSEndVertIndx += 2;
        }

        // compliance check
        if (epochIndxs[segFIndx] == epochIndxs[segSIndx] ||
            isOdds[segFIndx] != isOdds[segSIndx]) {
            continue;
        }
        // epochs checking
        auto segFEpoch = datasetPtr_->fromIndex(epochIndxs[segFIndx]);
        auto segSEpoch = datasetPtr_->fromIndex(epochIndxs[segSIndx]);
        if (!segFEpoch || !segSEpoch) {
            continue;
        }
        // isOdd checking
        bool segFIsOdd = isOdds[segFIndx] == '1';
        bool segSIsOdd = isOdds[segSIndx] == '1';
        if (segFIsOdd != segSIsOdd) {
            continue;
        }
        // segments checking
        auto segFCharts = segFEpoch->chart(segFIsOdd ? segSChannelId_ : segFChannelId_);
        auto segSCharts = segSEpoch->chart(segSIsOdd ? segSChannelId_ : segFChannelId_);
        if (!segFCharts || !segSCharts) {
            continue;
        }
        // dist procs checking
        if (!isfinite(segFEpoch->distProccesing(segFIsOdd ? segFChannelId_ : segSChannelId_)) ||
            !isfinite(segSEpoch->distProccesing(segSIsOdd ? segFChannelId_ : segSChannelId_))) {
            continue;
        }

        // Bresenham
        // first segment
        QVector3D segFBegPoint = !segFIsOdd ? vertices[segFBegVertIndx] : vertices[segFEndVertIndx];
        QVector3D segSEngPoint = !segFIsOdd ? vertices[segFEndVertIndx] : vertices[segFBegVertIndx];
        int segFX1 = std::min(matParams_.width_ - 1,  std::max(0, static_cast<int>(std::round((segFBegPoint.x() - matParams_.minX) * scaleFactor_))));
        int segFY1 = std::min(matParams_.height_ - 1, std::max(0, static_cast<int>(std::round((segFBegPoint.y() - matParams_.minY) * scaleFactor_))));
        int segFX2 = std::min(matParams_.width_ - 1,  std::max(0, static_cast<int>(std::round((segSEngPoint.x() - matParams_.minX) * scaleFactor_))));
        int segFY2 = std::min(matParams_.height_ - 1, std::max(0, static_cast<int>(std::round((segSEngPoint.y() - matParams_.minY) * scaleFactor_))));
        float segFPixTotDist = std::sqrt(std::pow(segFX2 - segFX1, 2) + std::pow(segFY2 - segFY1, 2));
        int segFDx = std::abs(segFX2 - segFX1);
        int segFDy = std::abs(segFY2 - segFY1);
        int segFSx = (segFX1 < segFX2) ? 1 : -1;
        int segFSy = (segFY1 < segFY2) ? 1 : -1;
        int segFErr = segFDx - segFDy;
        // second segment
        QVector3D segSBegPoint = !segSIsOdd ? vertices[segSBegVertIndx] : vertices[segSEndVertIndx];
        QVector3D segSEndPoint = !segSIsOdd ? vertices[segSEndVertIndx] : vertices[segSBegVertIndx];
        int segSX1 = std::min(matParams_.width_ - 1,  std::max(0, static_cast<int>(std::round((segSBegPoint.x() - matParams_.minX) * scaleFactor_))));
        int segSY1 = std::min(matParams_.height_ - 1, std::max(0, static_cast<int>(std::round((segSBegPoint.y() - matParams_.minY) * scaleFactor_))));
        int segSX2 = std::min(matParams_.width_ - 1,  std::max(0, static_cast<int>(std::round((segSEndPoint.x() - matParams_.minX) * scaleFactor_))));
        int segSY2 = std::min(matParams_.height_ - 1, std::max(0, static_cast<int>(std::round((segSEndPoint.y() - matParams_.minY) * scaleFactor_))));
        float segSPixTotDist = std::sqrt(std::pow(segSX2 - segSX1, 2) + std::pow(segSY2 - segSY1, 2));
        int segSDx = std::abs(segSX2 - segSX1);
        int segSDy = std::abs(segSY2 - segSY1);
        int segSSx = (segSX1 < segSX2) ? 1 : -1;
        int segSSy = (segSY1 < segSY2) ? 1 : -1;
        int segSErr = segSDx - segSDy;

        // pixel length checking
        if (!checkLength(segFPixTotDist) ||
            !checkLength(segSPixTotDist)) {
            continue;
        }

        while (true) { // line passing
            // first segment
            float segFPixCurrDist = std::sqrt(std::pow(segFX1 - segFX2, 2) + std::pow(segFY1 - segFY2, 2));
            float segFProgress = std::min(1.0f, segFPixCurrDist / segFPixTotDist);
            QVector3D segFPixPos(segFBegPoint.x() + segFProgress * (segSEngPoint.x() - segFBegPoint.x()),
                                 segFBegPoint.y() + segFProgress * (segSEngPoint.y() - segFBegPoint.y()),
                                 -1.0f * static_cast<float>(segFEpoch->distProccesing(segFIsOdd ? segFChannelId_ : segSChannelId_)));
            QVector3D segFBoatPos(segFEpoch->getPositionGNSS().ned.n, segFEpoch->getPositionGNSS().ned.e, 0.0f);
            auto segFColorIndx = getColorIndx(segFCharts, static_cast<int>(std::floor(segFPixPos.distanceToPoint(segFBoatPos) * amplitudeCoeff_)));

            // second segment, calc corresponding progress using smoothed interpolation
            float segSCorrProgress = std::min(1.0f, segFPixCurrDist / segFPixTotDist * segSPixTotDist / segFPixTotDist);
            QVector3D segSPixPos(segSBegPoint.x() + segSCorrProgress * (segSEndPoint.x() - segSBegPoint.x()),
                                 segSBegPoint.y() + segSCorrProgress * (segSEndPoint.y() - segSBegPoint.y()),
                                 -1.0f * static_cast<float>(segSEpoch->distProccesing(segSIsOdd ? segFChannelId_ : segSChannelId_)));
            QVector3D segSBoatPos(segSEpoch->getPositionGNSS().ned.n, segSEpoch->getPositionGNSS().ned.e, 0.0f);
            auto segSColorIndx  = getColorIndx(segSCharts, static_cast<int>(std::floor(segSPixPos.distanceToPoint(segSBoatPos) * amplitudeCoeff_)));

            if (sideScanLineDrawing) {
                uchar* pixPtr = imageData + segFY1 * bytesPerLine + segFX1 * bytesInPix;
                *pixPtr = colorTable_[segFColorIndx];
            }

            // interpolation between two pixels
            int interpX1 = std::min(matParams_.width_ - 1,  std::max(0, static_cast<int>(std::round((segFPixPos.x() - matParams_.minX) * scaleFactor_))));
            int interpY1 = std::min(matParams_.height_ - 1, std::max(0, static_cast<int>(std::round((segFPixPos.y() - matParams_.minY) * scaleFactor_))));
            int interpX2 = std::min(matParams_.width_ - 1,  std::max(0, static_cast<int>(std::round((segSPixPos.x() - matParams_.minX) * scaleFactor_))));
            int interpY2 = std::min(matParams_.height_ - 1, std::max(0, static_cast<int>(std::round((segSPixPos.y() - matParams_.minY) * scaleFactor_))));
            float interpPixTotDist = std::sqrt(std::pow(interpX2 - interpX1, 2) + std::pow(interpY2 - interpY1, 2));

            if (checkLength(interpPixTotDist)) {
                for (int step = 0; step <= interpPixTotDist; ++step) {
                    float interpProgress = static_cast<float>(step) / interpPixTotDist;
                    int interpX = interpX1 + interpProgress * (interpX2 - interpX1);
                    int interpY = interpY1 + interpProgress * (interpY2 - interpY1);
                    auto interpColorIndx = static_cast<int>((1 - interpProgress) * segFColorIndx + interpProgress * segSColorIndx);

                    for (int offsetX = -interpLineWidth; offsetX <= interpLineWidth; ++offsetX) {
                        for (int offsetY = -interpLineWidth; offsetY <= interpLineWidth; ++offsetY) {
                            int applyInterpX = std::min(matParams_.width_ - 1,  std::max(0, interpX + offsetX));
                            int applyInterpY = std::min(matParams_.height_ - 1, std::max(0, interpY + offsetY));
                            uchar* pixelPtr = imageData + applyInterpY * bytesPerLine + applyInterpX * bytesInPix;
                            *pixelPtr = colorTable_[interpColorIndx];
                        }
                    }
                }
            }

            // break at the end of the first segment
            if (segFX1 == segFX2 && segFY1 == segFY2)
                break;

            // Bresenham
            int segFE2 = 2 * segFErr;
            if (segFE2 > -segFDy) {
                segFErr -= segFDy;
                segFX1 += segFSx;
            }
            if (segFE2 < segFDx) {
                segFErr += segFDx;
                segFY1 += segFSy;
            }
            int segSE2 = 2 * segSErr;
            if (segSE2 > -segSDy) {
                segSErr -= segSDy;
                segSX1 += segSSx;
            }
            if (segSE2 < segSDx) {
                segSErr += segSDx;
                segSY1 += segSSy;
            }
        }
    }

    QTransform transform;
    transform.rotate(-90.0f);
    image_ = image_.transformed(transform);
}

SideScanView::MatrixParams SideScanView::getMatrixParams(const QVector<QVector3D> &vertices)
{
    MatrixParams retVal;

    if (vertices.isEmpty()) {
        return retVal;
    }

    auto [minX, maxX] = std::minmax_element(vertices.begin(), vertices.end(), [](const QVector3D &a, const QVector3D &b) { return a.x() < b.x(); });
    auto [minY, maxY] = std::minmax_element(vertices.begin(), vertices.end(), [](const QVector3D &a, const QVector3D &b) { return a.y() < b.y(); });

    retVal.minX = minX->x();
    retVal.maxX = maxX->x();
    retVal.minY = minY->y();
    retVal.maxY = maxY->y();
    retVal.width_  = static_cast<int>(std::ceil((maxX->x() - minX->x()) * scaleFactor_)) + 1;
    retVal.height_ = static_cast<int>(std::ceil((maxY->y() - minY->y()) * scaleFactor_)) + 1;

    return retVal;
}

int SideScanView::getColorIndx(Epoch::Echogram* charts, int ampIndx) const
{
    if (charts->amplitude.size() > ampIndx) {
        int cVal = charts->amplitude[ampIndx] * (1.5 + ampIndx * 0.0002f);
        cVal = std::min(colorTableSize_, cVal);
        return cVal;
    }
    return 0;
}

bool SideScanView::checkLength(float dist) const
{
    if (qFuzzyCompare(1.0f, 1.0f + dist) || (dist < 0.0f)) {
        return false;
    }
    return true;
}

void SideScanView::saveImageToFile(QImage &image, QString& path) const
{
    if (!image.save(path)) {
        qWarning() << "failed to save image at" << path;
    }
    else {
        qDebug() << "saved successfully at" << path;
    }
}


// SideScanViewRenderImplementation
SideScanView::SideScanViewRenderImplementation::SideScanViewRenderImplementation()
{

}

void SideScanView::SideScanViewRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp,
                                                            const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
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
