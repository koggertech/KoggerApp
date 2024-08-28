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

void SideScanView::updateData(bool sideScanLineDrawing, const QString& imagePath)
{
    if (!datasetPtr_) {
        qDebug() << "dataset is nullptr!";
        return;
    }

    clear();
    updateChannelsIds();

    ////////////////////////////////////////////////////////////////////
    // prepairing
    QVector<char> isOdds;
    QVector<int> epochIndxs;
    auto epochCount = datasetPtr_->size();
    auto renderImpl = RENDER_IMPL(SideScanView);

    renderImpl->measLinesVertices_.reserve(epochCount * 4);
    renderImpl->measLinesEvenIndices_.reserve(epochCount * 2);
    renderImpl->measLinesOddIndices_.reserve(epochCount * 2);
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
                renderImpl->measLinesVertices_.append(QVector3D(pos.n + lDist * qCos(leftAzRad), pos.e + lDist * qSin(leftAzRad),  0.0f));
                renderImpl->measLinesVertices_.append(QVector3D(pos.n, pos.e, 0.0f));
                renderImpl->measLinesEvenIndices_.append(currIndx++);
                renderImpl->measLinesEvenIndices_.append(currIndx++);
                isOdds.append('0');
                epochIndxs.append(i);
            }

            if (auto segSCharts = epoch->chart(segSChannelId_); segSCharts) {
                double rightAzRad = azRad + M_PI_2;
                float rDist = segSCharts ->range();
                renderImpl->measLinesVertices_.append(QVector3D(pos.n, pos.e, 0.0f));
                renderImpl->measLinesVertices_.append(QVector3D(pos.n + rDist * qCos(rightAzRad), pos.e + rDist * qSin(rightAzRad), 0.0f));
                renderImpl->measLinesOddIndices_.append(currIndx++);
                renderImpl->measLinesOddIndices_.append(currIndx++);
                isOdds.append('1');
                epochIndxs.append(i);
            }
        }
    }


    ////////////////////////////////////////////////////////////////////
    // processing
    // image
    auto imageMatParams = getMatrixParams(renderImpl->measLinesVertices_);
    image_ = QImage(imageMatParams.width, imageMatParams.height,  QImage::Format_Indexed8);
    image_.fill(Qt::black);
    uchar* imageData = image_.bits();
    int bytesPerLine = image_.bytesPerLine();
    int bytesInPix = bytesPerLine / image_.width();
    // height matrix
    const int hMatWidth = imageMatParams.unscaledWidth / heightStep_ + 1;
    const int hMatHeight = imageMatParams.unscaledHeight / heightStep_  + 1;
    QVector<QVector3D> heightVertices(hMatWidth * hMatHeight, QVector3D());
    for (int i = 0; i < hMatHeight; ++i) {
        for (int j = 0; j < hMatWidth; ++j) {
            float x = imageMatParams.minX + j * heightStep_;
            float y = imageMatParams.minY + i * heightStep_;
            heightVertices[i * hMatWidth + j] = QVector3D(x, y, 0.0f);
        }
    }

    // main cycle, interpolation only to the next epoch, when the segment side matches
    for (int i = 0; i < renderImpl->measLinesVertices_.size(); i += 2) { // 2 - step for 1 segment
        if (i + 8 >= renderImpl->measLinesVertices_.size()) {
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
        QVector3D segFBegPoint = !segFIsOdd ? renderImpl->measLinesVertices_[segFBegVertIndx] : renderImpl->measLinesVertices_[segFEndVertIndx];
        QVector3D segSEngPoint = !segFIsOdd ? renderImpl->measLinesVertices_[segFEndVertIndx] : renderImpl->measLinesVertices_[segFBegVertIndx];
        int segFX1 = std::min(imageMatParams.width  - 1, std::max(0, static_cast<int>(std::round((segFBegPoint.x() - imageMatParams.minX) * scaleFactor_))));
        int segFY1 = std::min(imageMatParams.height - 1, std::max(0, static_cast<int>(std::round((segFBegPoint.y() - imageMatParams.minY) * scaleFactor_))));
        int segFX2 = std::min(imageMatParams.width  - 1, std::max(0, static_cast<int>(std::round((segSEngPoint.x() - imageMatParams.minX) * scaleFactor_))));
        int segFY2 = std::min(imageMatParams.height - 1, std::max(0, static_cast<int>(std::round((segSEngPoint.y() - imageMatParams.minY) * scaleFactor_))));
        float segFPixTotDist = std::sqrt(std::pow(segFX2 - segFX1, 2) + std::pow(segFY2 - segFY1, 2));
        int segFDx = std::abs(segFX2 - segFX1);
        int segFDy = std::abs(segFY2 - segFY1);
        int segFSx = (segFX1 < segFX2) ? 1 : -1;
        int segFSy = (segFY1 < segFY2) ? 1 : -1;
        int segFErr = segFDx - segFDy;
        // second segment
        QVector3D segSBegPoint = !segSIsOdd ? renderImpl->measLinesVertices_[segSBegVertIndx] : renderImpl->measLinesVertices_[segSEndVertIndx];
        QVector3D segSEndPoint = !segSIsOdd ? renderImpl->measLinesVertices_[segSEndVertIndx] : renderImpl->measLinesVertices_[segSBegVertIndx];
        int segSX1 = std::min(imageMatParams.width  - 1, std::max(0, static_cast<int>(std::round((segSBegPoint.x() - imageMatParams.minX) * scaleFactor_))));
        int segSY1 = std::min(imageMatParams.height - 1, std::max(0, static_cast<int>(std::round((segSBegPoint.y() - imageMatParams.minY) * scaleFactor_))));
        int segSX2 = std::min(imageMatParams.width  - 1, std::max(0, static_cast<int>(std::round((segSEndPoint.x() - imageMatParams.minX) * scaleFactor_))));
        int segSY2 = std::min(imageMatParams.height - 1, std::max(0, static_cast<int>(std::round((segSEndPoint.y() - imageMatParams.minY) * scaleFactor_))));
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

        float segFDistProc = -1.0f * static_cast<float>(segFEpoch->distProccesing(segFIsOdd ? segFChannelId_ : segSChannelId_));
        float segSDistProc = -1.0f * static_cast<float>(segSEpoch->distProccesing(segSIsOdd ? segFChannelId_ : segSChannelId_));
        float segFDistX = segSEngPoint.x() - segFBegPoint.x();
        float segFDistY = segSEngPoint.y() - segFBegPoint.y();
        float segSDistX = segSEndPoint.x() - segSBegPoint.x();
        float segSDistY = segSEndPoint.y() - segSBegPoint.y();

        while (true) { // line passing
            // first segment
            float segFPixCurrDist = std::sqrt(std::pow(segFX1 - segFX2, 2) + std::pow(segFY1 - segFY2, 2));
            float segFProgress = std::min(1.0f, segFPixCurrDist / segFPixTotDist);
            QVector3D segFPixPos(segFBegPoint.x() + segFProgress * segFDistX, segFBegPoint.y() + segFProgress * segFDistY, segFDistProc);
            QVector3D segFBoatPos(segFEpoch->getPositionGNSS().ned.n, segFEpoch->getPositionGNSS().ned.e, 0.0f);
            auto segFColorIndx = getColorIndx(segFCharts, static_cast<int>(std::floor(segFPixPos.distanceToPoint(segFBoatPos) * amplitudeCoeff_)));

            // second segment, calc corresponding progress using smoothed interpolation
            float segSCorrProgress = std::min(1.0f, segFPixCurrDist / segFPixTotDist * segSPixTotDist / segFPixTotDist);
            QVector3D segSPixPos(segSBegPoint.x() + segSCorrProgress * segSDistX, segSBegPoint.y() + segSCorrProgress * segSDistY, segSDistProc);
            QVector3D segSBoatPos(segSEpoch->getPositionGNSS().ned.n, segSEpoch->getPositionGNSS().ned.e, 0.0f);
            auto segSColorIndx  = getColorIndx(segSCharts, static_cast<int>(std::floor(segSPixPos.distanceToPoint(segSBoatPos) * amplitudeCoeff_)));

            if (sideScanLineDrawing) {
                uchar* pixPtr = imageData + segFY1 * bytesPerLine + segFX1 * bytesInPix;
                *pixPtr = colorTable_[segFColorIndx];
            }

            // color interpolation between two pixels
            int interpX1 = std::min(imageMatParams.width  - 1, std::max(0, static_cast<int>(std::round((segFPixPos.x() - imageMatParams.minX) * scaleFactor_))));
            int interpY1 = std::min(imageMatParams.height - 1, std::max(0, static_cast<int>(std::round((segFPixPos.y() - imageMatParams.minY) * scaleFactor_))));
            int interpX2 = std::min(imageMatParams.width  - 1, std::max(0, static_cast<int>(std::round((segSPixPos.x() - imageMatParams.minX) * scaleFactor_))));
            int interpY2 = std::min(imageMatParams.height - 1, std::max(0, static_cast<int>(std::round((segSPixPos.y() - imageMatParams.minY) * scaleFactor_))));
            int interpDistX = interpX2 - interpX1;
            int interpDistY = interpY2 - interpY1;
            float interpPixTotDist = std::sqrt(std::pow(interpX2 - interpX1, 2) + std::pow(interpY2 - interpY1, 2));
            // height matrix
            int uinterpX1 = std::min(imageMatParams.unscaledWidth  - 1, std::max(0, static_cast<int>(std::round((segFPixPos.x() - imageMatParams.minX)))));
            int uinterpY1 = std::min(imageMatParams.unscaledHeight - 1, std::max(0, static_cast<int>(std::round((segFPixPos.y() - imageMatParams.minY)))));
            int uinterpX2 = std::min(imageMatParams.unscaledWidth  - 1, std::max(0, static_cast<int>(std::round((segSPixPos.x() - imageMatParams.minX)))));
            int uinterpY2 = std::min(imageMatParams.unscaledHeight - 1, std::max(0, static_cast<int>(std::round((segSPixPos.y() - imageMatParams.minY)))));
            int uinterpDistX = uinterpX2 - uinterpX1;
            int uinterpDistY = uinterpY2 - uinterpY1;

            if (checkLength(interpPixTotDist)) {
                for (int step = 0; step <= interpPixTotDist; ++step) {
                    float interpProgress = static_cast<float>(step) / interpPixTotDist;
                    int interpX = interpX1 + interpProgress * interpDistX;
                    int interpY = interpY1 + interpProgress * interpDistY;
                    auto interpColorIndx = static_cast<int>((1 - interpProgress) * segFColorIndx + interpProgress * segSColorIndx);                    
                    // height matrix
                    int uinterpX = uinterpX1 + interpProgress * uinterpDistX;
                    int uinterpY = uinterpY1 + interpProgress * uinterpDistY;

                    for (int offsetX = -interpLineWidth_; offsetX <= interpLineWidth_; ++offsetX) {
                        for (int offsetY = -interpLineWidth_; offsetY <= interpLineWidth_; ++offsetY) {
                            int applyInterpX = std::min(imageMatParams.width  - 1, std::max(0, interpX + offsetX));
                            int applyInterpY = std::min(imageMatParams.height - 1, std::max(0, interpY + offsetY));
                            uchar* pixPtr = imageData + applyInterpY * bytesPerLine + applyInterpX * bytesInPix;
                            *pixPtr = colorTable_[interpColorIndx];                            
                            // height matrix
                            int uApplyInterpX = std::min(imageMatParams.unscaledWidth  - 1, std::max(0, uinterpX + offsetX));
                            int uApplyInterpY = std::min(imageMatParams.unscaledHeight - 1, std::max(0, uinterpY + offsetY));
                            int heightVerticesIndx = (uApplyInterpY / heightStep_) * hMatWidth + (uApplyInterpX / heightStep_);
                            heightVertices[heightVerticesIndx][2] = segSPixPos.z();
                        }
                    }
                }
            }

            // break at the end of the first segment
            if (segFX1 == segFX2 && segFY1 == segFY2) {
                break;
            }

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

    // textureVertices, heightIndices
    QVector<QVector2D> textureVertices;
    QVector<int> heightIndices;
    for (int i = 0; i < hMatHeight ; ++i) {
        for (int j = 0; j < hMatWidth ; ++j) {
            textureVertices.append(QVector2D(float(j) / (hMatWidth - 1), float(i) / (hMatHeight - 1)));

            int topLeft = i * hMatWidth + j;
            int topRight = topLeft + 1;
            int bottomLeft = (i + 1) * hMatWidth + j;
            int bottomRight = bottomLeft + 1;

            if (qFuzzyCompare(1.0f, 1.0f + heightVertices[topLeft].z()) || // someone zero
                qFuzzyCompare(1.0f, 1.0f + heightVertices[topRight].z()) ||
                qFuzzyCompare(1.0f, 1.0f + heightVertices[bottomLeft].z()) ||
                qFuzzyCompare(1.0f, 1.0f + heightVertices[bottomRight].z())) {
                continue;
            }

            heightIndices.append(topLeft);     // 1--3
            heightIndices.append(bottomLeft);  // | /
            heightIndices.append(topRight);    // 2
            heightIndices.append(topRight);    //    1
            heightIndices.append(bottomLeft);  //  / |
            heightIndices.append(bottomRight); // 2--3
        }
    }

    // grid
    QVector<QVector3D> grid;
    for (int i = 0; i < heightIndices.size(); i += 6) {
        QVector3D A = heightVertices[heightIndices[i]];
        QVector3D B = heightVertices[heightIndices[i + 1]];
        QVector3D C = heightVertices[heightIndices[i + 2]];
        QVector3D D = heightVertices[heightIndices[i + 5]];
        A.setZ(A.z() + 0.02);
        B.setZ(B.z() + 0.02);
        C.setZ(C.z() + 0.02);
        D.setZ(D.z() + 0.02);
        grid.append({ A, B,
                      B, D,
                      A, C,
                      C, D });
    }

    renderImpl->heightVertices_ = heightVertices;
    renderImpl->textureVertices_ = textureVertices;
    renderImpl->heightIndices_ = heightIndices;
    renderImpl->gridRenderImpl_.setColor(QColor(0, 255, 0));
    renderImpl->gridRenderImpl_.setData(grid, GL_LINES);

    QTransform transform;
    transform.rotate(-90.0f);
    image_ = image_.transformed(transform);
    if (image_.save(imagePath)) {
        qDebug() << "image saved successfully at:" << imagePath;
    }

    Q_EMIT changed();    
}

void SideScanView::clear()
{
    auto renderImpl = RENDER_IMPL(SideScanView);

    renderImpl->measLinesVertices_.clear();
    renderImpl->measLinesEvenIndices_.clear();
    renderImpl->measLinesOddIndices_.clear();
    renderImpl->heightVertices_.clear();
    renderImpl->heightIndices_.clear();
    renderImpl->textureVertices_.clear();
    renderImpl->gridRenderImpl_.clearData();

    image_ = QImage();

    Q_EMIT changed();
}

void SideScanView::setScaleFactor(int scaleFactor)
{
    scaleFactor_ = static_cast<float>(scaleFactor);
}

void SideScanView::setMeasLineVisible(bool state)
{
    RENDER_IMPL(SideScanView)->measLineVisible_ = state;

    Q_EMIT changed();
}

void SideScanView::setDatasetPtr(Dataset* datasetPtr)
{
    datasetPtr_ = datasetPtr;
}

void SideScanView::setTextureId(GLuint textureId)
{
    RENDER_IMPL(SideScanView)->textureId_= textureId;

    Q_EMIT changed();
}

void SideScanView::setGridVisible(bool state)
{
    RENDER_IMPL(SideScanView)->gridVisible_ = state;

    Q_EMIT changed();
}

QImage& SideScanView::getImagePtr()
{
    return image_;
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

SideScanView::MatrixParams SideScanView::getMatrixParams(const QVector<QVector3D> &vertices) const
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

    retVal.unscaledWidth = static_cast<int>(std::ceil(maxX->x() - minX->x()));
    int ost = retVal.unscaledWidth % heightStep_;
    if (ost) {
        retVal.unscaledWidth = retVal.unscaledWidth - ost + heightStep_;
    }
    retVal.width = retVal.unscaledWidth * scaleFactor_;

    retVal.unscaledHeight = static_cast<int>(std::ceil(maxY->y() - minY->y()));
    ost = retVal.unscaledHeight % heightStep_;
    if (ost) {
        retVal.unscaledHeight = retVal.unscaledHeight - ost + heightStep_;
    }
    retVal.height = retVal.unscaledHeight * scaleFactor_;

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


/////////////////////////////////////////////////////////////////////////////////////////////////
// SideScanViewRenderImplementation
SideScanView::SideScanViewRenderImplementation::SideScanViewRenderImplementation() :
    textureId_(0),
    measLineVisible_(true),
    gridVisible_(false)
{

}

void SideScanView::SideScanViewRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp,
                                                            const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if (!m_isVisible) {
        return;
    }

    if (gridVisible_) {
        gridRenderImpl_.render(ctx, mvp, shaderProgramMap);
    }

    // measure lines
    if (measLineVisible_) {
        // static
        auto staticProgram = shaderProgramMap.value("static", nullptr);

        if (!staticProgram) {
            qWarning() << "Shader program 'static' not found!";
            return;
        }

        staticProgram->bind();
        staticProgram->setUniformValue("mvp", mvp);

        int posLoc = staticProgram->attributeLocation("position");
        staticProgram->enableAttributeArray(posLoc);

        staticProgram->setAttributeArray(posLoc, measLinesVertices_.constData());

        staticProgram->setUniformValue("color", QVector4D(0.0f, 1.0f, 0.0f, 1.0f));
        ctx->glDrawElements(GL_LINES, measLinesEvenIndices_.size(), GL_UNSIGNED_INT, measLinesEvenIndices_.constData());

        staticProgram->setUniformValue("color", QVector4D(1.0f, 0.0f, 0.0f, 1.0f));
        ctx->glDrawElements(GL_LINES, measLinesOddIndices_.size(), GL_UNSIGNED_INT, measLinesOddIndices_.constData());

        staticProgram->disableAttributeArray(posLoc);
        staticProgram->release();
    }

    // surface
    auto mosaicProgram = shaderProgramMap.value("mosaic", nullptr);

    if (!mosaicProgram) {
        qWarning() << "Shader program 'mosaic' not found!";
        return;
    }

    mosaicProgram->bind();
    mosaicProgram->setUniformValue("mvp", mvp);

    int newPosLoc = mosaicProgram->attributeLocation("position");
    int texCoordLoc = mosaicProgram->attributeLocation("texCoord");

    mosaicProgram->enableAttributeArray(newPosLoc);
    mosaicProgram->enableAttributeArray(texCoordLoc);

    mosaicProgram->setAttributeArray(newPosLoc , heightVertices_.constData());
    mosaicProgram->setAttributeArray(texCoordLoc, textureVertices_.constData());

    if (textureId_) {
        glBindTexture(GL_TEXTURE_2D, textureId_);
    }

    ctx->glDrawElements(GL_TRIANGLES, heightIndices_.size(), GL_UNSIGNED_INT, heightIndices_.constData());

    mosaicProgram->disableAttributeArray(texCoordLoc);
    mosaicProgram->disableAttributeArray(newPosLoc);
    mosaicProgram->release();
}
