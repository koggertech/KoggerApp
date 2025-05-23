#pragma once

#include <QObject>
#include <QVector>
#include <QImage>
#include <QPoint>
#include <QPixmap>
#include <QPainter>
#include <QEvent>


#include "plot2D_aim.h"
#include "plot2D_attitude.h"
#include "plot2D_bottom_processing.h"
#include "plot2D_contact.h"
#include "plot2D_defs.h"
#include "plot2D_dvl_beam_velocity.h"
#include "plot2D_dvl_solution.h"
#include "plot2D_echogram.h"
#include "plot2D_encoder.h"
#include "plot2D_gnss.h"
#include "plot2D_grid.h"
#include "plot2D_quadrature.h"
#include "plot2D_rangefinder.h"
#include "plot2D_usbl_solution.h"
#include "dataset.h"


class Plot2D
{
public:
    Plot2D();

    void setDataset(Dataset* dataset);

    float getDepthByMousePos(int mouseX, int mouseY, bool isHorizontal) const;
    int getEpochIndxByMousePos(int mouseX, int mouseY, bool isHorizontal) const;
    QPoint getMousePosByDepthAndEpochIndx(float depth, int epochIndx, bool isHorizontal) const;

    void addReRenderPlotIndxs(const QSet<int>& indxs);

    void setPlotEnabled(bool state);
    bool plotEnabled() const;

    bool isHorizontal();
    void setHorizontal(bool is_horizontal);

    void setAimEpochEventState(bool state);
    void setTimelinePosition(float position);
    void resetAim();

    void setTimelinePositionSec(float position);
    void setTimelinePositionByEpoch(int epochIndx);

    float timelinePosition();
    void scrollPosition(int columns);

    void setDataChannel(bool fromGui, const ChannelId& channel, uint8_t subChannel1, const QString& portName1, const ChannelId& channel2 = CHANNEL_NONE, uint8_t subChannel2 = 0, const QString& portName2 = QString());

    bool getIsContactChanged();

    QString getContactInfo();
    void    setContactInfo(const QString& str);
    bool    getContactVisible();
    void    setContactVisible(bool state);
    int     getContactPositionX();
    int     getContactPositionY();
    int     getContactIndx();
    double  getContactLat();
    double  getContactLon();
    double  getContactDepth();

    bool getImage(int width, int height, QPainter* painter, bool is_horizontal);
    void draw(QPainter* painterPtr);

    float getCursorDistance() const;
    std::tuple<ChannelId, uint8_t, QString> getSelectedChannelId(float cursorDistance = 0.0f) const;

    float getEchogramLowLevel() const;
    float getEchogramHighLevel() const;
    int getThemeId() const;
    void setEchogramLowLevel(float low);
    void setEchogramHightLevel(float high);
    void setEchogramVisible(bool visible);
    void setEchogramTheme(int theme_id);
    void setEchogramCompensation(int compensation_id);

    void setBottomTrackVisible(bool visible);
    void setBottomTrackTheme(int theme_id);

    void setRangefinderVisible(bool visible);
    void setRangefinderTheme(int theme_id);
    void setAttitudeVisible(bool visible);
    void setDopplerBeamVisible(bool visible, int beam_filter);
    void setDopplerInstrumentVisible(bool visible);

    void setGNSSVisible(bool visible, int flags);

    void setGridVetricalNumber(int grids);
    void setGridFillWidth(bool state);
    void setAngleVisibility(bool state);
    void setAngleRange(int angleRange);

    void setVelocityVisible(bool visible);
    void setVelocityRange(float velocity);
    void setDistanceAutoRange(int auto_range_type);

    void setDistance(float from, float to);
    void zoomDistance(float ratio);
    void scrollDistance(float ratio);

    void setMousePosition(int x, int y, bool isSync = false);
    void simpleSetMousePosition(int x, int y);
    void setMouseTool(MouseTool tool);
    bool setContact(int indx, const QString& text);
    bool deleteContact(int indx);
    void updateContact();
    void onCursorMoved(int x, int y);

    Canvas& canvas();
    DatasetCursor& cursor();

    void resetCash();
    Canvas image(int width, int height);
    void reindexingCursor();
    void reRangeDistance();

    virtual void plotUpdate();
    virtual void sendSyncEvent(int epoch_index, QEvent::Type eventType);

protected:
    Canvas canvas_;
    DatasetCursor cursor_;

    Plot2DAim aim_;
    Plot2DAttitude attitude_;
    Plot2DBottomProcessing bottomProcessing_;
    Plot2DContact contacts_;
    Plot2DDVLBeamVelocity dvlBeamVelocity_;
    Plot2DDVLSolution dvlSolution_;
    Plot2DEchogram echogram_;
    Plot2DEncoder encoder_;
    Plot2DGNSS gnss_;
    Plot2DGrid grid_;
    Plot2DQuadrature quadrature_;
    Plot2DRangefinder rangefinder_;
    Plot2DUSBLSolution usblSolution_;
    Dataset* datasetPtr_;
    std::function<void()> pendingBtpLambda_;
    bool isHorizontal_;

private:
    bool isEnabled_;
};
