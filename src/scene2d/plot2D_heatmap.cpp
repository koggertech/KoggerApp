#include "plot2D_heatmap.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <QDebug>
#include <QVector3D>

#include "DOA.h"
#include "plot2D.h"

namespace {

constexpr int kDoaSampleCount = 8;
constexpr int kAzimuthSamples = 360;
constexpr int kElevationSamples = 180;
constexpr int kMaxSensorCount = 5;

inline float wrapAzimuthDeg(float deg)
{
    while (deg > 180.0f) {
        deg -= 360.0f;
    }
    while (deg < -180.0f) {
        deg += 360.0f;
    }
    return deg;
}

struct DoaResult {
    bool valid = false;
    QVector<float> values;
    float peakAzimuthDeg = 0.0f;
    float peakElevationDeg = 0.0f;
};

template <int SensorCount>
bool runDoaTemplate(const std::array<float, SensorCount>& sensorX,
                    const std::array<float, SensorCount>& sensorY,
                    const std::array<float, SensorCount>& sensorZ,
                    const std::array<float, SensorCount * kDoaSampleCount>& antRe,
                    const std::array<float, SensorCount * kDoaSampleCount>& antIm,
                    float waveLen,
                    float* doaA,
                    float* doaE,
                    QVector<QVector<float>>& psMatrix)
{
    doa_hi_res<SensorCount, kDoaSampleCount>(sensorX.data(), sensorY.data(), sensorZ.data(),
                                             antRe.data(), antIm.data(),
                                             waveLen,
                                             doaA, doaE, psMatrix);
    return !psMatrix.isEmpty() && !psMatrix.first().isEmpty();
}

DoaResult computeDoaHeatMap(const QVector<ComplexSignal>& complexSignals,
                            int sensorCount,
                            const QVector<QVector3D>& sensorPositions)
{
    DoaResult result;

    if (sensorCount != 4 && sensorCount != 5) {
        return result;
    }

    if (complexSignals.size() < sensorCount || sensorPositions.size() < sensorCount) {
        return result;
    }

    int sampleCount = std::numeric_limits<int>::max();
    for (int i = 0; i < sensorCount; ++i) {
        sampleCount = std::min(sampleCount, static_cast<int>(complexSignals.at(i).data.size()));
    }

    if (sampleCount <= 0 || !std::isfinite(complexSignals.at(0).sampleRate) || complexSignals.at(0).sampleRate <= 0.0f) {
        return result;
    }

    if (sampleCount < kDoaSampleCount) {
        return result;
    }

    int maxSampleIndex = 0;
    float maxPower = -1.0f;
    for (int sample = 0; sample < sampleCount; ++sample) {
        float samplePower = 0.0f;
        for (int sensor = 0; sensor < sensorCount; ++sensor) {
            const ComplexF v = complexSignals.at(sensor).data.at(sample);
            samplePower += v.real * v.real + v.imag * v.imag;
        }

        if (samplePower > maxPower) {
            maxPower = samplePower;
            maxSampleIndex = sample;
        }
    }

    int startSample = maxSampleIndex - kDoaSampleCount / 2;
    startSample = std::clamp(startSample, 0, sampleCount - kDoaSampleCount);

    std::array<float, kMaxSensorCount * kDoaSampleCount> antRe = {};
    std::array<float, kMaxSensorCount * kDoaSampleCount> antIm = {};
    std::array<float, kMaxSensorCount> sensorX = {};
    std::array<float, kMaxSensorCount> sensorY = {};
    std::array<float, kMaxSensorCount> sensorZ = {};

    for (int sensor = 0; sensor < sensorCount; ++sensor) {
        const auto& complexSignal = complexSignals.at(sensor);
        const float phase = (sensor > 0 && sensor - 1 < 3) ? phase_corr[sensor - 1] : 0.0f;
        const float phaseCos = std::cos(phase);
        const float phaseSin = std::sin(phase);
        sensorX[sensor] = sensorPositions.at(sensor).x();
        sensorY[sensor] = sensorPositions.at(sensor).y();
        sensorZ[sensor] = sensorPositions.at(sensor).z();

        for (int sample = 0; sample < kDoaSampleCount; ++sample) {
            const ComplexF v = complexSignal.data.at(startSample + sample);
            const float re = v.real * phaseCos - v.imag * phaseSin;
            const float im = v.real * phaseSin + v.imag * phaseCos;
            antRe[sensor * kDoaSampleCount + sample] = re;
            antIm[sensor * kDoaSampleCount + sample] = im;
        }
    }

    const float waveLen = 1500.0f / complexSignals.at(0).sampleRate;
    if (!std::isfinite(waveLen) || waveLen <= 0.0f) {
        return result;
    }

    QVector<QVector<float>> psMatrix;
    float doaA = 0.0f;
    float doaE = 0.0f;
    bool ok = false;
    if (sensorCount == 4) {
        std::array<float, 4 * kDoaSampleCount> antRe4 = {};
        std::array<float, 4 * kDoaSampleCount> antIm4 = {};
        std::array<float, 4> sensorX4 = {};
        std::array<float, 4> sensorY4 = {};
        std::array<float, 4> sensorZ4 = {};
        std::copy_n(antRe.begin(), antRe4.size(), antRe4.begin());
        std::copy_n(antIm.begin(), antIm4.size(), antIm4.begin());
        std::copy_n(sensorX.begin(), sensorX4.size(), sensorX4.begin());
        std::copy_n(sensorY.begin(), sensorY4.size(), sensorY4.begin());
        std::copy_n(sensorZ.begin(), sensorZ4.size(), sensorZ4.begin());
        ok = runDoaTemplate<4>(sensorX4, sensorY4, sensorZ4, antRe4, antIm4, waveLen, &doaA, &doaE, psMatrix);
    } else if (sensorCount == 5) {
        std::array<float, 5 * kDoaSampleCount> antRe5 = {};
        std::array<float, 5 * kDoaSampleCount> antIm5 = {};
        std::array<float, 5> sensorX5 = {};
        std::array<float, 5> sensorY5 = {};
        std::array<float, 5> sensorZ5 = {};
        std::copy_n(antRe.begin(), antRe5.size(), antRe5.begin());
        std::copy_n(antIm.begin(), antIm5.size(), antIm5.begin());
        std::copy_n(sensorX.begin(), sensorX5.size(), sensorX5.begin());
        std::copy_n(sensorY.begin(), sensorY5.size(), sensorY5.begin());
        std::copy_n(sensorZ.begin(), sensorZ5.size(), sensorZ5.begin());
        ok = runDoaTemplate<5>(sensorX5, sensorY5, sensorZ5, antRe5, antIm5, waveLen, &doaA, &doaE, psMatrix);
    }

    if (!ok) {
        return result;
    }

    const int rows = psMatrix.size();
    const int cols = psMatrix.first().size();
    if (rows != kElevationSamples || cols != kAzimuthSamples) {
        return result;
    }
    result.values.resize(rows * cols);

    float maxVal = 0.0f;
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            const float value = psMatrix.at(y).at(x);
            result.values[y * cols + x] = value;
            maxVal = std::max(maxVal, value);
        }
    }

    result.valid = maxVal > 0.0f;
    result.peakAzimuthDeg = wrapAzimuthDeg(doaA * 180.0f / static_cast<float>(PI));
    result.peakElevationDeg = doaE * 180.0f / static_cast<float>(PI);
    return result;
}

} // namespace

Plot2DHeatMap::Plot2DHeatMap()
{
    setVisible(true);
    setThemeId(6);
    setLevels(10.0f, 100.0f);
    sensorPositions_ = {
        QVector3D(sen_x[0], sen_y[0], sen_z[0]),
        QVector3D(sen_x[1], sen_y[1], sen_z[1]),
        QVector3D(sen_x[2], sen_y[2], sen_z[2]),
        QVector3D(sen_x[3], sen_y[3], sen_z[3]),
        QVector3D(0.0f, 0.0f, 0.0f)
    };
}

bool Plot2DHeatMap::draw(Plot2D* parent, Dataset* dataset)
{
    if (!parent) {
        clearState();
        return false;
    }

    return draw(parent, dataset, QRect(0, 0, parent->canvas().width(), parent->canvas().height()));
}

bool Plot2DHeatMap::draw(Plot2D* parent, Dataset* dataset, const QRect& targetRect)
{
    if (!isVisible() || !parent || !dataset) {
        clearState();
        return false;
    }

    if (!rebuild(parent, dataset) || image_.isNull()) {
        return false;
    }

    if (pixmapDirty_) {
        pixmap_ = QPixmap::fromImage(image_);
        pixmapDirty_ = false;
    }

    auto& canvas = parent->canvas();
    if (canvas.width() <= 0 || canvas.height() <= 0 || !targetRect.isValid()) {
        return false;
    }

    canvas.painter()->drawPixmap(targetRect,
                                 pixmap_,
                                 QRect(0, 0, pixmap_.width(), pixmap_.height()));

    QPainter* painter = canvas.painter();
    painter->save();
    painter->setClipRect(targetRect);

    QPen minorGrid(QColor(220, 240, 255, 70));
    minorGrid.setWidth(1);
    painter->setPen(minorGrid);

    constexpr int kVerticalCells = 8;
    constexpr int kHorizontalCells = 8;

    for (int i = 1; i < kVerticalCells; ++i) {
        const int x = targetRect.left() + i * targetRect.width() / kVerticalCells;
        painter->drawLine(x, targetRect.top(), x, targetRect.bottom());
    }

    for (int i = 1; i < kHorizontalCells; ++i) {
        const int y = targetRect.top() + i * targetRect.height() / kHorizontalCells;
        painter->drawLine(targetRect.left(), y, targetRect.right(), y);
    }

    QPen majorGrid(QColor(235, 250, 255, 120));
    majorGrid.setWidth(1);
    painter->setPen(majorGrid);
    const int centerX = targetRect.left() + targetRect.width() / 2;
    const int centerY = targetRect.top() + targetRect.height() / 2;
    painter->drawLine(centerX, targetRect.top(), centerX, targetRect.bottom());
    painter->drawLine(targetRect.left(), centerY, targetRect.right(), centerY);
    painter->restore();

    return true;
}

void Plot2DHeatMap::setThemeId(int themeId)
{
    themeId_ = themeId;

    QVector<QColor> colors;
    QVector<int> levels;

    switch (themeId) {
    case 1:
        colors = { QColor::fromRgb(0, 0, 0), QColor::fromRgb(50, 50, 10), QColor::fromRgb(230, 200, 100), QColor::fromRgb(255, 255, 220) };
        levels = { 0, 30, 130, 255 };
        break;
    case 2:
        colors = {
            QColor::fromRgb(0, 0, 0), QColor::fromRgb(28, 10, 0), QColor::fromRgb(55, 18, 0), QColor::fromRgb(95, 35, 0),
            QColor::fromRgb(150, 70, 10), QColor::fromRgb(210, 105, 15), QColor::fromRgb(245, 175, 70), QColor::fromRgb(255, 232, 160)
        };
        levels = { 0, 12, 26, 52, 92, 140, 200, 255 };
        break;
    case 3:
        colors = {
            QColor::fromRgb(0, 0, 0), QColor::fromRgb(40, 0, 80), QColor::fromRgb(0, 30, 150),
            QColor::fromRgb(20, 230, 30), QColor::fromRgb(255, 50, 20), QColor::fromRgb(255, 255, 255)
        };
        levels = { 0, 30, 80, 120, 150, 255 };
        break;
    case 4:
        colors = { QColor::fromRgb(0, 0, 0), QColor::fromRgb(190, 200, 200), QColor::fromRgb(230, 255, 255) };
        levels = { 0, 150, 255 };
        break;
    case 5:
        colors = { QColor::fromRgb(230, 255, 255), QColor::fromRgb(70, 70, 70), QColor::fromRgb(0, 0, 0) };
        levels = { 0, 150, 255 };
        break;
    case 6:
        colors = {
            QColor::fromRgb(0, 0, 40), QColor::fromRgb(20, 0, 120), QColor::fromRgb(40, 0, 200),
            QColor::fromRgb(0, 90, 255), QColor::fromRgb(0, 200, 255), QColor::fromRgb(0, 255, 200),
            QColor::fromRgb(0, 255, 80), QColor::fromRgb(220, 255, 0), QColor::fromRgb(255, 120, 0),
            QColor::fromRgb(255, 40, 40), QColor::fromRgb(255, 255, 255)
        };
        levels = { 0, 18, 36, 60, 90, 120, 150, 185, 210, 235, 255 };
        break;
    case 7:
        colors = {
            QColor::fromRgb(0, 0, 70), QColor::fromRgb(0, 40, 150), QColor::fromRgb(0, 100, 230),
            QColor::fromRgb(0, 180, 255), QColor::fromRgb(0, 240, 255), QColor::fromRgb(80, 255, 230),
            QColor::fromRgb(170, 255, 255), QColor::fromRgb(230, 255, 255), QColor::fromRgb(255, 255, 255)
        };
        levels = { 0, 30, 60, 95, 130, 170, 210, 235, 255 };
        break;
    case 8:
        colors = {
            QColor::fromRgb(0, 0, 0), QColor::fromRgb(0, 50, 0), QColor::fromRgb(0, 100, 0), QColor::fromRgb(0, 160, 0),
            QColor::fromRgb(0, 220, 0), QColor::fromRgb(0, 255, 0), QColor::fromRgb(120, 255, 0), QColor::fromRgb(200, 255, 0),
            QColor::fromRgb(255, 255, 80), QColor::fromRgb(255, 255, 255)
        };
        levels = { 0, 20, 45, 70, 100, 130, 160, 190, 220, 255 };
        break;
    case 9:
        colors = {
            QColor::fromRgb(51, 128, 255), QColor::fromRgb(51, 102, 230), QColor::fromRgb(77, 77, 204),
            QColor::fromRgb(102, 51, 179), QColor::fromRgb(128, 51, 153), QColor::fromRgb(153, 77, 128),
            QColor::fromRgb(179, 102, 102), QColor::fromRgb(204, 128, 77), QColor::fromRgb(230, 153, 51),
            QColor::fromRgb(255, 179, 26)
        };
        levels = { 0, 28, 56, 84, 112, 140, 168, 196, 224, 255 };
        break;
    default:
        colors = { QColor::fromRgb(0, 0, 0), QColor::fromRgb(20, 5, 80), QColor::fromRgb(50, 180, 230), QColor::fromRgb(190, 240, 250), QColor::fromRgb(255, 255, 255) };
        levels = { 0, 30, 130, 220, 255 };
        break;
    }

    setColorScheme(colors, levels);
}

void Plot2DHeatMap::setLevels(float low, float high)
{
    lowLevel_ = low;
    highLevel_ = high;
    updateColors();
}

void Plot2DHeatMap::resetCache()
{
    cachedEpochIndex_ = -1;
    cachedChannelId_.clear();
    cachedSubChannelId_ = 0;
    pixmapDirty_ = true;
}

void Plot2DHeatMap::setSensorCount(int sensorCount)
{
    const int bounded = qBound(4, sensorCount, 5);
    if (sensorCount_ == bounded) {
        return;
    }

    sensorCount_ = bounded;
    resetCache();
}

int Plot2DHeatMap::sensorCount() const
{
    return sensorCount_;
}

void Plot2DHeatMap::setSensorPosition(int sensorIndex, const QVector3D& position)
{
    if (sensorIndex < 0 || sensorIndex >= sensorPositions_.size()) {
        return;
    }

    if (!std::isfinite(position.x()) || !std::isfinite(position.y()) || !std::isfinite(position.z())) {
        return;
    }

    if (sensorPositions_.at(sensorIndex) == position) {
        return;
    }

    sensorPositions_[sensorIndex] = position;
    resetCache();
}

QVector3D Plot2DHeatMap::sensorPosition(int sensorIndex) const
{
    if (sensorIndex < 0 || sensorIndex >= sensorPositions_.size()) {
        return QVector3D();
    }

    return sensorPositions_.at(sensorIndex);
}

const Plot2DHeatMap::State& Plot2DHeatMap::state() const
{
    return state_;
}

void Plot2DHeatMap::clearState()
{
    state_ = State{};
    image_ = QImage();
    pixmap_ = QPixmap();
    cachedEpochIndex_ = -1;
    cachedChannelId_.clear();
    cachedSubChannelId_ = 0;
    pixmapDirty_ = true;
}

void Plot2DHeatMap::logStatus(const QString& message)
{
    if (lastLogMessage_ == message) {
        return;
    }

    lastLogMessage_ = message;
    qInfo().noquote() << "[HeatMap]" << message;
}

void Plot2DHeatMap::updateColors()
{
    if (colorTable_.isEmpty()) {
        return;
    }

    colorLevels_.resize(256);

    const float levelRange = highLevel_ - lowLevel_;
    const int indexOffset = static_cast<int>(lowLevel_ * 2.5f);
    const float indexScale = levelRange > 0.0f ? static_cast<float>(255) / (levelRange * 2.55f) : 10000.0f;

    for (int i = 0; i < colorTable_.size(); ++i) {
        int mappedIndex = static_cast<int>((static_cast<float>(i - indexOffset)) * indexScale);
        mappedIndex = std::clamp(mappedIndex, 0, 255);
        colorLevels_[i] = colorTable_.at(mappedIndex);
    }

    if (!image_.isNull()) {
        image_.setColorTable(colorLevels_);
    }

    colorDirty_ = false;
    pixmapDirty_ = true;
}

void Plot2DHeatMap::setColorScheme(const QVector<QColor>& colors, const QVector<int>& levels)
{
    if (colors.size() != levels.size() || colors.size() < 2) {
        return;
    }

    colorTable_.resize(256);
    colorLevels_.resize(256);

    int levelIndex = 0;
    for (int i = 0; i < colors.size() - 1; ++i) {
        while (levelIndex <= levels.at(i + 1) && levelIndex < 256) {
            const float b = static_cast<float>(levelIndex - levels.at(i)) / static_cast<float>(levels.at(i + 1) - levels.at(i));
            const float a = 1.0f - b;
            const int red = qRound(colors.at(i).red() * a + colors.at(i + 1).red() * b);
            const int green = qRound(colors.at(i).green() * a + colors.at(i + 1).green() * b);
            const int blue = qRound(colors.at(i).blue() * a + colors.at(i + 1).blue() * b);
            colorTable_[levelIndex] = qRgb(red, green, blue);
            ++levelIndex;
        }
    }

    while (levelIndex < 256) {
        colorTable_[levelIndex] = colorTable_.at(255);
        ++levelIndex;
    }

    updateColors();
}

bool Plot2DHeatMap::rebuild(Plot2D* parent, Dataset* dataset)
{
    auto& cursor = parent->cursor();

    if (colorDirty_) {
        updateColors();
    }

    if (cursor.currentEpochIndx < 0) {
        logStatus(QStringLiteral("No current epoch. Move cursor over plot."));
        clearState();
        return false;
    }

    const auto [channelId, subChannelId, name] = parent->getSelectedChannelId();
    Q_UNUSED(name);

    auto* epoch = dataset->fromIndex(cursor.currentEpochIndx);
    if (!epoch || !epoch->isComplexSignalAvail()) {
        logStatus(QString("Epoch %1 has no ComplexData").arg(cursor.currentEpochIndx));
        clearState();
        return false;
    }

    auto& complexSignals = epoch->complexSignals();
    auto channelIt = complexSignals.find(channelId);
    if (channelIt == complexSignals.end()) {
        logStatus(QString("Epoch %1 missing complex channel uuid=%2 addr=%3")
                  .arg(cursor.currentEpochIndx)
                  .arg(channelId.uuid.toString(QUuid::WithoutBraces))
                  .arg(channelId.address));
        clearState();
        return false;
    }

    auto groupIt = channelIt->find(channelId.address);
    if (groupIt == channelIt->end()) {
        logStatus(QString("Epoch %1 missing complex group %2 for selected channel")
                  .arg(cursor.currentEpochIndx)
                  .arg(channelId.address));
        clearState();
        return false;
    }

    const QVector<ComplexSignal>& complexGroup = groupIt.value();

    if (cursor.currentEpochIndx == cachedEpochIndex_
        && cachedChannelId_ == channelId
        && cachedSubChannelId_ == subChannelId
        && !image_.isNull()) {
        state_.valid = true;
        logStatus(QString("Using cached HeatMap for epoch=%1 channelAddr=%2 sub=%3")
                  .arg(cursor.currentEpochIndx)
                  .arg(channelId.address)
                  .arg(subChannelId));
        return true;
    }

    const DoaResult doa = computeDoaHeatMap(complexGroup, sensorCount_, sensorPositions_);
    const int azimuthSamples = kAzimuthSamples;
    const int elevationSamples = kElevationSamples;

    if (!doa.valid || doa.values.size() != azimuthSamples * elevationSamples) {
        logStatus(QString("DOA build failed for epoch=%1 channelAddr=%2 sub=%3 sensors=%4")
                  .arg(cursor.currentEpochIndx)
                  .arg(channelId.address)
                  .arg(subChannelId)
                  .arg(sensorCount_));
        clearState();
        return false;
    }

    image_ = QImage(azimuthSamples, elevationSamples, QImage::Format_Indexed8);
    image_.setColorTable(colorLevels_);

    float minValue = std::numeric_limits<float>::max();
    float maxValue = std::numeric_limits<float>::lowest();
    QVector<float> logValues(doa.values.size(), 0.0f);

    for (int i = 0; i < doa.values.size(); ++i) {
        const float value = std::max(doa.values.at(i), std::numeric_limits<float>::epsilon());
        const float logValue = 10.0f * std::log10(value);
        logValues[i] = logValue;
        minValue = std::min(minValue, logValue);
        maxValue = std::max(maxValue, logValue);
    }

    const float valueRange = std::max(maxValue - minValue, std::numeric_limits<float>::epsilon());

    for (int y = 0; y < elevationSamples; ++y) {
        uchar* scanLine = image_.scanLine(elevationSamples - 1 - y);
        for (int x = 0; x < azimuthSamples; ++x) {
            const int sourceX = (x + azimuthSamples / 2) % azimuthSamples;
            const float logValue = logValues.at(y * azimuthSamples + sourceX);
            const float norm = (logValue - minValue) / valueRange;
            scanLine[x] = static_cast<uchar>(std::clamp(qRound(norm * 255.0f), 0, 255));
        }
    }

    state_.valid = true;
    state_.epochIndex = cursor.currentEpochIndx;
    state_.channelId = channelId;
    state_.subChannelId = subChannelId;
    state_.azimuthFromDeg = -180.0f;
    state_.azimuthToDeg = 180.0f;
    state_.elevationFromDeg = -90.0f;
    state_.elevationToDeg = 90.0f;
    state_.peakAzimuthDeg = doa.peakAzimuthDeg;
    state_.peakElevationDeg = doa.peakElevationDeg;

    cachedEpochIndex_ = cursor.currentEpochIndx;
    cachedChannelId_ = channelId;
    cachedSubChannelId_ = subChannelId;
    pixmapDirty_ = true;
    logStatus(QString("Built HeatMap epoch=%1 channelAddr=%2 sub=%3 sensors=%4 peakAz=%5 peakEl=%6")
              .arg(cursor.currentEpochIndx)
              .arg(channelId.address)
              .arg(subChannelId)
              .arg(sensorCount_)
              .arg(state_.peakAzimuthDeg, 0, 'f', 1)
              .arg(state_.peakElevationDeg, 0, 'f', 1));
    return true;
}
