#pragma once

#include "plot2D_plot_layer.h"
#include "plot2D_defs.h"


class Plot2DEchogram : public PlotLayer {
public:
    enum ThemeId {
        ClassicTheme,
        SepiaTheme,
        WRGBDTheme,
        WBTheme,
        BWTheme
    };

    Plot2DEchogram();
    bool draw(Plot2D* parent, Dataset* dataset);

    float getLowLevel() const;
    float getHighLevel() const;
    void setLowLevel(float low);
    void setHightLevel(float high);
    void setLevels(float low, float hight);

    void setColorScheme(QVector<QColor> coloros, QVector<int> levels);
    int getThemeId() const;
    void setThemeId(int theme_id);
    void setCompensation(int compensation_id);

    void updateColors();

    int updateCash(Plot2D* parent, Dataset* dataset, int width, int height);
    void resetCash();

    void addReRenderPlotIndxs(const QSet<int>& indxs);

protected:
    typedef struct {
        typedef enum {
            CashStateNotValid,
            CashStateValid,
            CashStateEraced
        } CashState;


        int poolIndex = -1;
        CashState state = CashStateNotValid;
        bool isNeedUpdate = true;

        QVector<int16_t> data;

//        CashState stateColor = CashStateNotValid;
//        QVector<uint16_t> color;
    } CashLine;

    uint16_t _colorHashMap[256];
    QVector<CashLine> _cash;

    QVector<QRgb> _colorTable;
    QVector<QRgb> _colorLevels;
    QImage _image;
    QPixmap _pixmap;
    bool _flagColorChanged = true;

    int _compensation_id = 0;

    struct {
        bool resetCash = true;
    } _cashFlags;

    struct {
       float low = 100, high = 10;
    } _levels;

    struct {
       float low = NAN, high = NAN;
    } _lastLevels;

    DatasetCursor _lastCursor;
    int _lastWidth = -1;
    int _lastHeight = -1;

    bool getTriggerCashReset() {
        bool reset_cash = _cashFlags.resetCash;
        _cashFlags.resetCash = false;
        return reset_cash;
    }
private:
    ThemeId themeId_;
    QSet<int> reRenderPlotIndxs_;
};
