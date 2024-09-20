#include "draw_utils.h"


sscan::PlotColorTable::PlotColorTable()
{
    setThemeById(static_cast<int>(ThemeId::kClassic));
    setLevels(10.0f, 100.0f);
}

void sscan::PlotColorTable::setThemeById(int id)
{
    ThemeId theme;
    if (id >= static_cast<int>(ThemeId::kClassic) && id <= static_cast<int>(ThemeId::kBW)) {
        theme = static_cast<ThemeId>(id);
    }
    else {
        return;
    }

    QVector<QColor> colors;
    QVector<int> levels;

    switch (theme) {
    case ThemeId::kClassic: {
        colors = { QColor::fromRgb(0,   0,   0),
                   QColor::fromRgb(20,  5,   80),
                   QColor::fromRgb(50,  180, 230),
                   QColor::fromRgb(190, 240, 250),
                   QColor::fromRgb(255, 255, 255) };
        levels = { 0, 30, 130, 220, 255 };
        break;
    }
    case ThemeId::kSepia: {
        colors = { QColor::fromRgb(0,   0,   0),
                   QColor::fromRgb(50,  50,  10),
                   QColor::fromRgb(230, 200, 100),
                   QColor::fromRgb(255, 255, 220) };
        levels = { 0, 30, 130, 255 };
        break;
    }
    case ThemeId::kWRGBD: {
        colors = { QColor::fromRgb(0,   0,   0),
                   QColor::fromRgb(40,  0,   80),
                   QColor::fromRgb(0,   30,  150),
                   QColor::fromRgb(20,  230, 30),
                   QColor::fromRgb(255, 50,  20),
                   QColor::fromRgb(255, 255, 255) };
        levels = { 0, 30, 80, 120, 150, 255 };
        break;
    }
    case ThemeId::kWB: {
        colors = { QColor::fromRgb(0,   0,   0),
                   QColor::fromRgb(190, 200, 200),
                   QColor::fromRgb(230, 255, 255) };
        levels = { 0, 150, 255 };
        break;
    }
    case ThemeId::kBW: {
        colors = { QColor::fromRgb(230, 255, 255),
                   QColor::fromRgb(70,  70,  70),
                   QColor::fromRgb(0,   0,   0) };
        levels = { 0, 150, 255 };
        break;
    }
    default:
        break;
    }

    setColorScheme(colors, levels);
}

void sscan::PlotColorTable::setLevels(float low, float high)
{
    lowLevel_ = low;
    highLevel_ = high;

    update();
}

void sscan::PlotColorTable::setLowLevel(float val)
{
    setLevels(val, highLevel_);
}

void sscan::PlotColorTable::setHighLevel(float val)
{
    setLevels(lowLevel_, val);
}

QVector<QRgb> sscan::PlotColorTable::getColorTable() const
{
    return colorTableWithLevels_;
}

void sscan::PlotColorTable::update()
{
    int levelRange = highLevel_ - lowLevel_;
    int indexOffset = static_cast<int>(lowLevel_ * 2.5f);
    float indexMapScale = 0;

    if (levelRange > 0) {
        indexMapScale = static_cast<float>((256 - 1) / ((highLevel_ - lowLevel_) * 2.55f));
    }
    else {
        indexMapScale = 10000.0f;
    }

    for (int i = 0; i < colorTable_.size(); i++) {
        int indexMap = static_cast<int>((i - indexOffset) * indexMapScale);

        if (indexMap < 0) {
            indexMap = 0;
        }
        else if (indexMap > 255) {
            indexMap = 255;
        }

        colorTableWithLevels_[i] = colorTable_[indexMap];
    }
}

void sscan::PlotColorTable::setColorScheme(const QVector<QColor> &colors, const QVector<int> &levels)
{
    if (colors.length() != levels.length()) {
        return;
    }

    colorTable_.resize(256);
    colorTableWithLevels_.resize(256);

    int nbrLevels = colors.length() - 1;
    int iLevel = 0;

    for (int i = 0; i < nbrLevels; ++i) {
        while (levels[i + 1] >= iLevel) {
            float bCoef = static_cast<float>((iLevel - levels[i])) / static_cast<float>((levels[i + 1] - levels[i]));
            float aCoef = 1.0f - bCoef;

            int red   = qRound(colors[i].red()   * aCoef + colors[i + 1].red()   * bCoef);
            int green = qRound(colors[i].green() * aCoef + colors[i + 1].green() * bCoef);
            int blue  = qRound(colors[i].blue()  * aCoef + colors[i + 1].blue()  * bCoef);

            colorTable_[iLevel] = qRgb(red, green, blue);
            ++iLevel;
        }
    }

    update();
}
