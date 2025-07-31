#include "draw_utils.h"


mosaic::PlotColorTable::PlotColorTable()
{
    setTheme(static_cast<int>(ThemeId::kClassic));
    setLevels(10.0f, 100.0f);
}

void mosaic::PlotColorTable::setTheme(int id)
{
    ThemeId theme;
    if (id >= static_cast<int>(ThemeId::kClassic) && id <= static_cast<int>(ThemeId::kBW)) {
        theme = static_cast<ThemeId>(id);
    }
    else {
        return;
    }

    themeId_ = static_cast<int>(theme);

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

void mosaic::PlotColorTable::setLevels(float low, float high)
{
    lowLevel_ = low;
    highLevel_ = high;

    update();
}

void mosaic::PlotColorTable::setLowLevel(float val)
{
    setLevels(val, highLevel_);
}

void mosaic::PlotColorTable::setHighLevel(float val)
{
    setLevels(lowLevel_, val);
}

QVector<QRgb> mosaic::PlotColorTable::getColorTable() const
{
    return colorTableWithLevels_;
}

std::vector<uint8_t> mosaic::PlotColorTable::getRgbaColors() const
{
    return rgbaColors_;
}

void mosaic::PlotColorTable::update()
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

    int colorCount = colorTableWithLevels_.size();
    rgbaColors_.resize(colorCount * 4);
    for (int i = 0; i < colorCount; ++i) {
        QRgb color = colorTableWithLevels_[i];
        rgbaColors_[i * 4 + 0] = static_cast<uint8_t>(qRed(color));
        rgbaColors_[i * 4 + 1] = static_cast<uint8_t>(qGreen(color));
        rgbaColors_[i * 4 + 2] = static_cast<uint8_t>(qBlue(color));
        rgbaColors_[i * 4 + 3] = static_cast<uint8_t>(qAlpha(color));
    }
}

void mosaic::PlotColorTable::setColorScheme(const QVector<QColor> &colors, const QVector<int> &levels)
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

int mosaic::PlotColorTable::getTheme() const
{
    return themeId_;
}

std::pair<float, float> mosaic::PlotColorTable::getLevels() const
{
    return std::make_pair(lowLevel_, highLevel_);
}

float mosaic::PlotColorTable::getLowLevel() const
{
    return lowLevel_;
}

float mosaic::PlotColorTable::getHighLevel() const
{
    return highLevel_;
}
