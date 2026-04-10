#pragma once

#include <QImage>
#include <QPixmap>
#include <QString>
#include <QVector>
#include <QVector3D>

#include "plot2D_plot_layer.h"

class Plot2DHeatMap : public PlotLayer
{
public:
    struct State {
        bool valid = false;
        int epochIndex = -1;
        ChannelId channelId;
        uint8_t subChannelId = 0;
        float azimuthFromDeg = -180.0f;
        float azimuthToDeg = 180.0f;
        float elevationFromDeg = -90.0f;
        float elevationToDeg = 90.0f;
        float peakAzimuthDeg = 0.0f;
        float peakElevationDeg = 0.0f;
    };

    Plot2DHeatMap();

    bool draw(Plot2D* parent, Dataset* dataset) override;
    bool draw(Plot2D* parent, Dataset* dataset, const QRect& targetRect);

    void setThemeId(int themeId);
    void setLevels(float low, float high);
    void resetCache();
    void setSensorCount(int sensorCount);
    int sensorCount() const;
    void setSensorPosition(int sensorIndex, const QVector3D& position);
    QVector3D sensorPosition(int sensorIndex) const;

    const State& state() const;

private:
    void clearState();
    void logStatus(const QString& message);
    void updateColors();
    void setColorScheme(const QVector<QColor>& colors, const QVector<int>& levels);
    bool rebuild(Plot2D* parent, Dataset* dataset);

    QVector<QRgb> colorTable_;
    QVector<QRgb> colorLevels_;
    QVector<QVector3D> sensorPositions_;
    QImage image_;
    QPixmap pixmap_;
    State state_;
    int themeId_ = 0;
    float lowLevel_ = 10.0f;
    float highLevel_ = 100.0f;
    int sensorCount_ = 4;
    bool colorDirty_ = true;
    bool pixmapDirty_ = true;
    int cachedEpochIndex_ = -1;
    ChannelId cachedChannelId_;
    uint8_t cachedSubChannelId_ = 0;
    QString lastLogMessage_;
};
