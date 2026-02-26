#pragma once

#include <QImage>
#include <QQuickPaintedItem>
#include <QObject>
#include <dataset.h>
#include <QTimer>
#include "plot2D.h"

//#define USE_PIXMAP


class qPlot2D : public QQuickPaintedItem, public Plot2D
{
    Q_OBJECT
public:
    Q_PROPERTY(bool horizontal READ isHorizontal() WRITE setHorizontal)
    Q_PROPERTY(float timelinePosition READ timelinePosition WRITE setTimelinePosition NOTIFY timelinePositionChanged)
    Q_PROPERTY(bool isEnabled READ getPlotEnabled WRITE setPlotEnabled)
    Q_PROPERTY(QString contactInfo      READ getContactInfo      WRITE setContactInfo     NOTIFY contactChanged)
    Q_PROPERTY(bool    contactVisible   READ getContactVisible   WRITE setContactVisible  NOTIFY contactChanged)
    Q_PROPERTY(int     contactPositionX READ getContactPositionX /*WRITE setContactPosition*/ NOTIFY contactChanged)
    Q_PROPERTY(int     contactPositionY READ getContactPositionY /*WRITE setContactPosition*/ NOTIFY contactChanged)
    Q_PROPERTY(int     contactIndx      READ getContactIndx /*WRITE setContactIndx*/ NOTIFY contactChanged)
    Q_PROPERTY(double  contactLat       READ getContactLat /*WRITE setContactLat*/ NOTIFY contactChanged)
    Q_PROPERTY(double  contactLon       READ getContactLon /*WRITE setContactLon*/ NOTIFY contactChanged)
    Q_PROPERTY(double  contactDepth     READ getContactDepth /*WRITE setContactLon*/ NOTIFY contactChanged)

    qPlot2D(QQuickItem* parent = nullptr);
    void paint(QPainter *painter) override;
//    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;

    void setPlot(Dataset* plot);
    void setDataProcessor(DataProcessor* dataProcessorPtr);

    bool isHorizontal() { return _isHorizontal; }
    void setHorizontal(bool is_horizontal) { _isHorizontal = is_horizontal; Plot2D::setHorizontal(_isHorizontal); update(); }

    void plotUpdate() override;

    bool eventFilter(QObject *watched, QEvent *event) override final;
    void sendSyncEvent(int epoch_index, QEvent::Type eventType) override final;

    Q_INVOKABLE float cursorFrom() const { return Plot2D::cursor_.distance.from; }
    Q_INVOKABLE float cursorTo() const { return Plot2D::cursor_.distance.to; }
    Q_INVOKABLE void setCursorFromTo(float from, float to) { cursor_.distance.mode = AutoRangeNone; Plot2D::cursor_.distance.from = from; Plot2D::cursor_.distance.to = to; }
    Q_INVOKABLE void setTimelinePositionByEpochCentered(int epochIndx) {
        if (!datasetPtr_ || datasetPtr_->size() <= 0) {
            return;
        }
        if (epochIndx < 0) {
            Plot2D::setTimelinePosition(cursor_.position);
            cursor_.selectEpochIndx = -1;
            return;
        }
        const float halfWidth = static_cast<float>(qRound(width() * 0.5f));
        const float timelinePos = static_cast<float>(epochIndx + halfWidth) / static_cast<float>(datasetPtr_->size());
        Plot2D::setTimelinePosition(qBound(0.0f, timelinePos, 1.0f));
        cursor_.selectEpochIndx = epochIndx;
    }
    Q_INVOKABLE void setZoomPreviewMode(bool enabled) {
        if (zoomPreviewMode_ == enabled) {
            return;
        }
        zoomPreviewMode_ = enabled;
        update();
    }
    Q_INVOKABLE void setZoomPreviewSourceSize(int sourceSize) {
        const int bounded = qBound(4, sourceSize, 4096);
        if (zoomPreviewSourceSize_ == bounded) {
            return;
        }
        zoomPreviewSourceSize_ = bounded;
        update();
    }
    Q_INVOKABLE void setZoomPreviewReferenceDepthPixels(int pixels) {
        const int bounded = qMax(0, pixels);
        if (zoomPreviewReferenceDepthPixels_ == bounded) {
            return;
        }
        zoomPreviewReferenceDepthPixels_ = bounded;
        update();
    }
    Q_INVOKABLE void setZoomPreviewSourceByEpochDepth(int epochIndx, float depth) {
        if (zoomPreviewEpochIndx_ == epochIndx && qFuzzyCompare(zoomPreviewDepth_ + 1.0f, depth + 1.0f)) {
            return;
        }
        zoomPreviewEpochIndx_ = epochIndx;
        zoomPreviewDepth_ = depth;
        update();
    }
    Q_INVOKABLE void setZoomPreviewFlipY(bool enabled) {
        if (zoomPreviewFlipY_ == enabled) {
            return;
        }
        zoomPreviewFlipY_ = enabled;
        update();
    }
    Q_INVOKABLE void setIndx(int indx) { indx_ = indx; }

protected:
    Dataset* m_plot = nullptr;
    QTimer* m_updateTimer;
    bool m_needUpdate = true;
    bool _isHorizontal = true;

signals:
    void timelinePositionChanged();
    void contactChanged();
    void plotHorizontalChanged();

protected slots:
    void timerUpdater();
    void dataUpdate() { plotUpdate(); }

public slots:
    void updater();
    void horScrollEvent(int delta);
    void verZoomEvent(int delta);
    void verScrollEvent(int delta);
    Q_INVOKABLE void plotMousePosition(int x, int y, bool isSync = false);
    Q_INVOKABLE void simplePlotMousePosition(int x, int y);
    Q_INVOKABLE void onCursorMoved(int x, int y);
    Q_INVOKABLE void plotMouseTool(int mode);
    Q_INVOKABLE bool setContact(int indx, const QString& text);
    Q_INVOKABLE bool setActiveContact(int indx);
    Q_INVOKABLE bool deleteContact(int indx);
    Q_INVOKABLE void updateContact();
    void setPlotEnabled(bool state);

    Q_INVOKABLE void mosaicLOffsetChanged(float val);
    Q_INVOKABLE void mosaicROffsetChanged(float val);

    void plotDatasetChannelFromStrings(const QString& ch1Str, const QString& ch2Str)
    {
        if (!datasetPtr_) {
           return;
        }

        auto [ch1, sub1, name1] = datasetPtr_->channelIdFromName(ch1Str);
        auto [ch2, sub2, name2] = datasetPtr_->channelIdFromName(ch2Str);

        setDataChannel(true, ch1, sub1, name1, ch2, sub2, name2);

        plotUpdate();
    }

    ChannelId plotDatasetChannel() { return cursor_.channel1; }
    uint8_t plotDatasetSubChannel() { return cursor_.subChannel1; }
    ChannelId plotDatasetChannel2() { return cursor_.channel2; }
    uint8_t plotDatasetSubChannel2() { return cursor_.subChannel2; }
    Q_INVOKABLE QString plotDatasetChannelName() const { return cursor_.firstChannelPortName; }
    Q_INVOKABLE QString plotDatasetChannel2Name() const { return cursor_.secondChannelPortName; }

    void plotEchogramVisible(bool visible) { setEchogramVisible(visible); }
    Q_INVOKABLE void plotEchogramTheme(int theme_id) { setEchogramTheme(theme_id); }
    Q_INVOKABLE void plotEchogramCompensation(int compensation_id) { setEchogramCompensation(compensation_id); }
    void plotBottomTrackVisible(bool visible) { setBottomTrackVisible(visible); }
    void plotBottomTrackTheme(int theme_id) { setBottomTrackTheme(theme_id); }
    void plotBottomTrackDepthTextVisible(bool visible) { setBottomTrackDepthTextVisible(visible); }

    void plotRangefinderVisible(bool visible) { setRangefinderVisible(visible); }
    void plotRangefinderTheme(int theme_id) { setRangefinderTheme(theme_id); }
    void plotRangefinderDepthTextVisible(bool visible) { setRangefinderDepthTextVisible(visible); }
    void plotAttitudeVisible(bool visible) { setAttitudeVisible(visible); }
    void plotTemperatureVisible(bool visible) { setTemperatureVisible(visible); }
    void plotDopplerBeamVisible(bool visible, int beam_filter) { setDopplerBeamVisible(visible, beam_filter); }
    void plotDopplerInstrumentVisible(bool visible) { setDopplerInstrumentVisible(visible); }

    void plotGNSSVisible(bool visible, int flags) { setGNSSVisible(visible, flags);}

    void plotAcousticAngleVisible(bool visible) { setAcousticAngleVisible(visible); }

    void plotGridVerticalNumber(int grids) { setGridVetricalNumber(grids); }
    void plotGridFillWidth(bool state) { setGridFillWidth(state); };
    void plotGridInvert(bool state) { setGridInvert(state); };
    void plotAngleVisibility(bool state)   { setAngleVisibility(state); }
    void plotAngleRange(int angleRange) { setAngleRange(angleRange); }
    void plotVelocityVisible(bool visible) { setVelocityVisible(visible); }
    void plotVelocityRange(float velocity) { setVelocityRange(velocity); }

    void plotDistanceAutoRange(int auto_range_type) { setDistanceAutoRange(auto_range_type); }
    void plotLoupeVisible(bool state) { setLoupeVisible(state); }
    void plotLoupeSize(int size) { setLoupeSize(size); }
    void plotLoupeZoom(int zoom) { setLoupeZoom(zoom); }

    void plotEchogramSetLevels(float low, float hight) {
        setEchogramLowLevel(low);
        setEchogramHightLevel(hight);
    }

    Q_INVOKABLE float getLowEchogramLevel() const;
    Q_INVOKABLE float getHighEchogramLevel() const;
    Q_INVOKABLE int getThemeId() const;
    Q_INVOKABLE int getEchogramCompensation() const { return Plot2D::getEchogramCompensation(); }
    Q_INVOKABLE float getLoupeDepthForEpoch(int epochIndx) const;
    void doDistProcessing(int preset, int window_size, float vertical_gap, float range_min, float range_max, float gain_slope, float threshold, float offsetx, float offsety, float offsetz, bool manual);
    void refreshDistParams(int preset, int windowSize, float verticalGap, float rangeMin, float rangeMax, float gainSlope, float threshold, float offsetX, float offsetY, float offsetZ);

    void setPreset(int value);
    void setWindowSize(int value);
    void setVerticalGap(float value);
    void setRangeMin(float value);
    void setRangeMax(float value);
    void setGainSlope(float value);
    void setThreshold(float value);
    void setOffsetX(float value);
    void setOffsetY(float value);
    void setOffsetZ(float value);

private:
    int indx_ = -1;
    bool zoomPreviewMode_ = false;
    int zoomPreviewSourceSize_ = 44;
    int zoomPreviewReferenceDepthPixels_ = 0;
    int zoomPreviewEpochIndx_ = -1;
    float zoomPreviewDepth_ = 0.0f;
    bool zoomPreviewFlipY_ = false;
};
