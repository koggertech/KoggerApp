#include "plot2D_echogram.h"
#include "plot2D.h"

namespace {
class MiniPreviewPlot2D final : public Plot2D {
public:
    MiniPreviewPlot2D()
    {
        setHorizontal(true);
        setPlotEnabled(true);
    }

    bool render(QPainter* painter,
                Dataset* dataset,
                const DatasetCursor& parentCursor,
                int parentCanvasWidth,
                int sourceLeft,
                int sourceWidth,
                int previewWidth,
                int previewHeight,
                float zoomFrom,
                float zoomTo,
                int themeId,
                float lowLevel,
                float highLevel,
                int compensationId)
    {
        if (!painter || !dataset || previewWidth <= 0 || previewHeight <= 0 || parentCanvasWidth <= 0) {
            return false;
        }

        setDataset(dataset);
        canvas_.setSize(previewWidth, previewHeight, painter);

        cursor_ = parentCursor;
        cursor_.distance.mode = AutoRangeNone;
        cursor_.distance.from = zoomFrom;
        cursor_.distance.to = zoomTo;
        cursor_.setMouse(-1, -1);
        cursor_.selectEpochIndx = -1;
        cursor_.currentEpochIndx = -1;
        cursor_.lastEpochIndx = -1;

        const int fallbackX = qBound(0, parentCanvasWidth / 2, parentCanvasWidth - 1);
        int fallbackEpoch = parentCursor.getIndex(fallbackX);
        if (dataset->validIndex(fallbackEpoch) < 0) {
            fallbackEpoch = -1;
        }

        int lastValidEpoch = fallbackEpoch;
        int zeroEpochCount = 0;
        const int maxParentX = parentCanvasWidth - 1;

        cursor_.indexes.resize(previewWidth);
        for (int column = 0; column < previewWidth; ++column) {
            const float srcXFloat = static_cast<float>(sourceLeft)
                + (static_cast<float>(column) + 0.5f) * static_cast<float>(sourceWidth) / static_cast<float>(previewWidth);
            const int sourceX = qBound(0, qRound(srcXFloat), maxParentX);

            int epochIndex = parentCursor.getIndex(sourceX);
            if (dataset->validIndex(epochIndex) < 0) {
                epochIndex = lastValidEpoch;
            }
            else {
                lastValidEpoch = epochIndex;
            }

            if (dataset->validIndex(epochIndex) < 0) {
                ++zeroEpochCount;
            }

            cursor_.indexes[column] = epochIndex;
        }

        cursor_.numZeroEpoch = zeroEpochCount;

        echogram_.setVisible(true);
        echogram_.setThemeId(themeId);
        echogram_.setLevels(lowLevel, highLevel);
        echogram_.setCompensation(compensationId);

        return echogram_.draw(this, dataset);
    }
};
} // namespace


Plot2DEchogram::Plot2DEchogram()
{
    setThemeId(ClassicTheme);
    setLevels(10, 100);
}

void Plot2DEchogram::setLowLevel(float low)
{
    setLevels(low, _levels.high);
}

void Plot2DEchogram::setHightLevel(float high)
{
    setLevels(_levels.low, high);
}

void Plot2DEchogram::setLevels(float low, float high)
{
    _levels.low = low;
    _levels.high = high;
    updateColors();
}

void Plot2DEchogram::setColorScheme(QVector<QColor> coloros, QVector<int> levels)
{
    if(coloros.length() != levels.length()) { return; }

    _colorTable.resize(256);
    _colorLevels.resize(256);

    int nbr_levels = coloros.length() - 1;
    int i_level = 0;

    for(int i = 0; i < nbr_levels; i++) {
        while(levels[i + 1] >= i_level) {
            float b_koef = (float)(i_level - levels[i]) / (float)(levels[i + 1] - levels[i]);
            float a_koef = 1.0f - b_koef;

            int red = qRound(coloros[i].red()*a_koef + coloros[i + 1].red()*b_koef);
            int green = qRound(coloros[i].green()*a_koef + coloros[i + 1].green()*b_koef);
            int blue = qRound(coloros[i].blue()*a_koef + coloros[i + 1].blue()*b_koef);
            _colorHashMap[i_level] = ((red / 8) << 10) | ((green / 8) << 5) | ((blue / 8));

            _colorTable[i_level] = qRgb(red, green, blue);
            i_level++;
        }
    }

    updateColors();
}

int Plot2DEchogram::getThemeId() const
{
    return static_cast<int>(themeId_);
}

void Plot2DEchogram::setThemeId(int theme_id) {
    if (theme_id >= ClassicTheme && theme_id <= MidnightTheme) {
        themeId_ = static_cast<ThemeId>(theme_id);
    }
    else {
        themeId_ = ClassicTheme;
    }

    QVector<QColor> coloros;
    QVector<int> levels;

    if(theme_id == ClassicTheme) {
        coloros = { QColor::fromRgb(0, 0, 0), QColor::fromRgb(20, 5, 80), QColor::fromRgb(50, 180, 230), QColor::fromRgb(190, 240, 250), QColor::fromRgb(255, 255, 255)};
        levels = {0, 30, 130, 220, 255};
    } else if(theme_id == SepiaTheme) {
        coloros = { QColor::fromRgb(0,   0,   0),
                    QColor::fromRgb(50,  50,  10),
                    QColor::fromRgb(230, 200, 100),
                    QColor::fromRgb(255, 255, 220) };
        levels = {0, 30, 130, 255};
    } else if(theme_id == SepiaNewTheme) {
        coloros = {
            QColor::fromRgb(  0,   0,   0),
            QColor::fromRgb( 28,  10,   0),
            QColor::fromRgb( 55,  18,   0),
            QColor::fromRgb( 95,  35,   0),
            QColor::fromRgb(150,  70,  10),
            QColor::fromRgb(210, 105,  15),
            QColor::fromRgb(245, 175,  70),
            QColor::fromRgb(255, 232, 160)
        };
        levels = { 0, 12, 26, 52, 92, 140, 200, 255 };
    }else if(theme_id == WRGBDTheme) {
        coloros = {
            QColor::fromRgb(0, 0, 0),
            QColor::fromRgb(40, 0, 80),
            QColor::fromRgb(0, 30, 150),
            QColor::fromRgb(20, 230, 30),
            QColor::fromRgb(255, 50, 20),
            QColor::fromRgb(255, 255, 255),
        };

        levels = {0, 30, 80, 120, 150, 255};
    } else if(theme_id == WBTheme) {
        coloros = { QColor::fromRgb(0, 0, 0), QColor::fromRgb(190, 200, 200), QColor::fromRgb(230, 255, 255)};
        levels = {0, 150, 255};
    } else if(theme_id == BWTheme) {
        coloros = {QColor::fromRgb(230, 255, 255), QColor::fromRgb(70, 70, 70), QColor::fromRgb(0, 0, 0)};
        levels = {0, 150, 255};
    } else if(theme_id == DeepBlueTheme) {
        coloros = { QColor::fromRgb(0,   0,   40),
                    QColor::fromRgb(20,  0,   120),
                    QColor::fromRgb(40,  0,   200),
                    QColor::fromRgb(0,   90,  255),
                    QColor::fromRgb(0,   200, 255),
                    QColor::fromRgb(0,   255, 200),
                    QColor::fromRgb(0,   255, 80),
                    QColor::fromRgb(220, 255, 0),
                    QColor::fromRgb(255, 120, 0),
                    QColor::fromRgb(255, 40,  40),
                    QColor::fromRgb(255, 255, 255) };
        levels = {0, 18, 36, 60, 90, 120, 150, 185, 210, 235, 255};
    } else if(theme_id == IceTheme) {
        coloros = { QColor::fromRgb(0,   0,   70),
                    QColor::fromRgb(0,   40,  150),
                    QColor::fromRgb(0,   100, 230),
                    QColor::fromRgb(0,   180, 255),
                    QColor::fromRgb(0,   240, 255),
                    QColor::fromRgb(80,  255, 230),
                    QColor::fromRgb(170, 255, 255),
                    QColor::fromRgb(230, 255, 255),
                    QColor::fromRgb(255, 255, 255) };
        levels = {0, 30, 60, 95, 130, 170, 210, 235, 255};
    } else if(theme_id == GreenTheme) {
        coloros = { QColor::fromRgb(0,   0,   0),
                    QColor::fromRgb(0,   50,  0),
                    QColor::fromRgb(0,   100, 0),
                    QColor::fromRgb(0,   160, 0),
                    QColor::fromRgb(0,   220, 0),
                    QColor::fromRgb(0,   255, 0),
                    QColor::fromRgb(120, 255, 0),
                    QColor::fromRgb(200, 255, 0),
                    QColor::fromRgb(255, 255, 80),
                    QColor::fromRgb(255, 255, 255) };
        levels = {0, 20, 45, 70, 100, 130, 160, 190, 220, 255};
    } else if(theme_id == MidnightTheme) {
        coloros = { QColor::fromRgb(51,  128, 255),
                    QColor::fromRgb(51,  102, 230),
                    QColor::fromRgb(77,  77,  204),
                    QColor::fromRgb(102, 51,  179),
                    QColor::fromRgb(128, 51,  153),
                    QColor::fromRgb(153, 77,  128),
                    QColor::fromRgb(179, 102, 102),
                    QColor::fromRgb(204, 128, 77),
                    QColor::fromRgb(230, 153, 51),
                    QColor::fromRgb(255, 179, 26) };
        levels = {0, 28, 56, 84, 112, 140, 168, 196, 224, 255};
    }

    setColorScheme(coloros, levels);
}

void Plot2DEchogram::setCompensation(int compensation_id)
{
    _compensation_id = compensation_id;
}

void Plot2DEchogram::updateColors()
{
    float low = _levels.low;
    float high = _levels.high;

    int level_range = high - low;
    int index_offset = (int)((float)low*2.5f);
    float index_map_scale = 0;
    if(level_range > 0) {
        index_map_scale = (float)(256 - 1)/((float)(high - low)*2.55f);
    } else {
        index_map_scale = 10000;
    }

    for(int i = 0; i < _colorTable.size(); i++) {
        int index_map = ((float)(i - index_offset)*index_map_scale);
        if(index_map < 0) { index_map = 0; }
        else if(index_map > 255) { index_map = 255; }
        _colorLevels[i] = _colorTable[index_map];
    }

    _flagColorChanged = true;
    _image.setColorTable(_colorLevels);
}

void Plot2DEchogram::resetCash()
{
    _cashFlags.resetCash = true;
}

void Plot2DEchogram::addReRenderPlotIndxs(const QSet<int> &indxs)
{
    reRenderPlotIndxs_.unite(indxs);
}

int Plot2DEchogram::updateCash(Plot2D* parent, Dataset* dataset, int width, int height)
{
    auto& cursor = parent->cursor();

    if (_cash.size() != width) {
        _cash.resize(width);
        resetCash();
    }

    uint8_t* image_data = (uint8_t*)_image.constBits();
    const int b_scanline = _image.bytesPerLine();

    bool is_cash_notvalid = getTriggerCashReset();
    is_cash_notvalid |= !_lastCursor.isChannelsEqual(cursor);
    is_cash_notvalid |= !_lastCursor.isDistanceEqual(cursor);
    is_cash_notvalid |=  _lastWidth != width;
    is_cash_notvalid |=  _lastHeight != height;


    float from = cursor.distance.from;
    float to = cursor.distance.to;
    float fullrange = to - from;

    float range1 = 0;
    float from1 = 0;
    float to1 = 0;

    float from2 = 0;
    float to2 = 0;

    if (to >= 0) {
        range1 = 0 - from;
        from1 = 0;
        to1 = -from;

        if (from >= 0) {
            from2 = from;
        }
        else {
            from2 = 0;
        }
        to2 = to;
    }
    else {
        range1 = to - from;
        from1 = -to;
        to1 = -from;
    }

    int wrap_start_pos = qAbs(cursor.getIndex(0) % width);

    for (unsigned int i = 0; i < cursor.indexes.size(); i++) {
        if (cursor.indexes[i] > 0) {
            wrap_start_pos = qAbs((cursor.indexes[i] + (width - i)) % width);
            break;
        }
    }


//    _cashPosition = wrap_start_pos;
    for(int column = 0; column < width; column++) {
        if(_cash[column].data.size() != height) {
//            _cash[column].stateColor = CashLine::CashState::CashStateNotValid;
            _cash[column].state = CashLine::CashState::CashStateNotValid;
            _cash[column].data.resize(height);
//            _cash[column].data.fill(0);
            _cash[column].poolIndex = -1;
            _cash[column].state = CashLine::CashState::CashStateEraced;
            _cash[column].isNeedUpdate = true;

            int16_t cash_data_size = _cash[column].data.size();
            int16_t* cash_data = _cash[column].data.data();
            uint8_t * img_data = image_data + column;
            for (int image_row = 0; image_row < cash_data_size; image_row++) {
                *img_data = *cash_data;
                img_data += b_scanline;
                cash_data++;
            }
        }

        int cursor_pos = column - wrap_start_pos;
        if(column < wrap_start_pos) {
            cursor_pos += width;
        }

        int pool_index = cursor.getIndex(cursor_pos);
        int pool_index_safe = dataset->validIndex(pool_index);
        if(pool_index_safe >= 0) {

            bool wasValidlyRendered = true;
            if (reRenderPlotIndxs_.contains(pool_index_safe)) {
                reRenderPlotIndxs_.remove(pool_index_safe);
                wasValidlyRendered = false;
            }

            auto* datasource = dataset->fromIndex(pool_index_safe);
            const int cash_index = _cash[column].poolIndex;

            if (is_cash_notvalid || pool_index_safe != cash_index || !wasValidlyRendered) {
                _cash[column].poolIndex = pool_index_safe;

                if(datasource != NULL) {
                    _cash[column].state = CashLine::CashState::CashStateNotValid;
                    int16_t* cash_data = _cash[column].data.data();
                    int16_t cash_data_size = _cash[column].data.size();

                    if (cursor.channel2 == CHANNEL_NONE) {
                        datasource->chartTo(cursor.channel1, cursor.subChannel1, from, to, cash_data, cash_data_size, _compensation_id);
                    }
                    else {
                        int cash_data_size_part1 = cash_data_size*(range1/fullrange);

                        if(cash_data_size_part1 > 0) {
                            datasource->chartTo(cursor.channel1, cursor.subChannel1, from1, to1, cash_data, cash_data_size_part1, _compensation_id, true);
                        }

                        if(cash_data_size_part1 < 0) {
                            cash_data_size_part1 = 0;
                        }

                        const int cash_data_size_part2 = cash_data_size - cash_data_size_part1;
                        if(cash_data_size_part2 > 0) {
                            datasource->chartTo(cursor.channel2, cursor.subChannel2, from2, to2, &cash_data[cash_data_size_part1], cash_data_size_part2, _compensation_id, false);
                        }
                    }

                    _cash[column].state = CashLine::CashState::CashStateValid;
                    _cash[column].isNeedUpdate = true;
                    uint8_t * img_data = image_data + column;
                    for (int image_row = 0; image_row < cash_data_size; image_row++) {
                        *img_data = *cash_data;
                        img_data += b_scanline;
                        cash_data++;
                    }
//                    _cash[column].stateColor = CashLine::CashState::CashStateNotValid;
                } else {
                    if(_cash[column].state != CashLine::CashState::CashStateEraced) {
//                        _cash[column].stateColor = CashLine::CashState::CashStateNotValid;
                        _cash[column].state = CashLine::CashState::CashStateNotValid;
                        _cash[column].data.fill(0);
                        _cash[column].poolIndex = -1;
                        _cash[column].state = CashLine::CashState::CashStateEraced;
                        _cash[column].isNeedUpdate = true;

                        int16_t cash_data_size = _cash[column].data.size();
                        int16_t* cash_data = _cash[column].data.data();
                        uint8_t * img_data = image_data + column;
                        for (int image_row = 0; image_row < cash_data_size; image_row++) {
                            *img_data = *cash_data;
                            img_data += b_scanline;
                            cash_data++;
                        }
                    }
                }

            }
        } else {
            if(_cash[column].state != CashLine::CashState::CashStateEraced) {
//                _cash[column].stateColor = CashLine::CashState::CashStateNotValid;
                _cash[column].state = CashLine::CashState::CashStateNotValid;
                _cash[column].data.fill(0);
                _cash[column].poolIndex = -1;
                _cash[column].state = CashLine::CashState::CashStateEraced;
                _cash[column].isNeedUpdate = true;

                int16_t* cash_data = _cash[column].data.data();
                int16_t cash_data_size = _cash[column].data.size();
                uint8_t * img_data = image_data + column;
                for (int image_row = 0; image_row < cash_data_size; image_row++) {
                    *img_data = *cash_data;
                    img_data += b_scanline;
                    cash_data++;
                }
            }

        }
    }

    //qInfo("Cash validate %u", cash_validate);

    _lastCursor = cursor;
    _lastWidth = width;
    _lastHeight = height;

    return wrap_start_pos;
}

bool Plot2DEchogram::draw(Plot2D* parent, Dataset* dataset)
{
    auto& canvas = parent->canvas();
    auto& cursor = parent->cursor();

    if (isVisible() && dataset != nullptr && cursor.distance.isValid()) {
        const int image_width = canvas.width();
        const int image_height = canvas.height();

        if(_image.width() != image_width || _image.height() != image_height) {
            _image = QImage(image_width, image_height, QImage::Format_Indexed8);
            _image.setColorTable(_colorLevels);
            _pixmap = QPixmap(image_width, image_height);
        }

        const int cash_width = canvas.width();

        const int cash_position = updateCash(parent, dataset, cash_width, image_height);

        QPainter p(&_pixmap);

        int cash_col = 0;
        while(cash_col < cash_width) {
            int cash_col_1 = cash_col;
            while(cash_col < cash_width && (_cash[cash_col].isNeedUpdate || _flagColorChanged)) {
                _cash[cash_col].isNeedUpdate = false;
                cash_col++;
            }

            int cash_update_width = cash_col - cash_col_1;

            if(cash_update_width > 0) {
                 p.drawImage(cash_col_1, 0, _image, cash_col_1, 0 , cash_update_width, 0, Qt::ThresholdDither); // Qt::NoOpaqueDetection |
            } else {
                cash_col++;
            }
        }

        _flagColorChanged = false;

        canvas.painter()->drawPixmap(0, 0, _pixmap, cash_position, 0, cash_width - cash_position, 0);
        canvas.painter()->drawPixmap(cash_width - cash_position, 0, _pixmap, 0, 0, cash_position, 0);
    } else {
    }

    return true;
}

bool Plot2DEchogram::drawZoomPreview(Plot2D* parent, Dataset* dataset, QPainter* painter, const QRect& targetRect, const QPoint& sourceCenter, int sourceSize)
{
    if (!parent || !dataset || !painter || targetRect.width() <= 0 || targetRect.height() <= 0) {
        return false;
    }

    auto& cursor = parent->cursor();
    auto& canvas = parent->canvas();

    if (!cursor.distance.isValid() || canvas.width() <= 0 || canvas.height() <= 0) {
        return false;
    }

    const int previewWidth = targetRect.width();
    const int previewHeight = targetRect.height();
    if (previewWidth <= 0 || previewHeight <= 0) {
        return false;
    }

    const int srcWidth = qBound(4, sourceSize, canvas.width());
    const int srcHeight = qBound(4, sourceSize, canvas.height());

    const int clampedCenterX = qBound(0, sourceCenter.x(), canvas.width() - 1);
    const int clampedCenterY = qBound(0, sourceCenter.y(), canvas.height() - 1);

    const float cursorFrom = cursor.distance.from;
    const float cursorTo = cursor.distance.to;
    const float cursorRange = cursorTo - cursorFrom;
    if (qFuzzyIsNull(cursorRange)) {
        return false;
    }

    const float centerScale = static_cast<float>(clampedCenterY) / static_cast<float>(canvas.height());
    const float centerDistance = cursorFrom + centerScale * cursorRange;
    float distanceSpan = std::abs(cursorRange) * (static_cast<float>(srcHeight) / static_cast<float>(canvas.height()));
    if (distanceSpan < 0.01f) {
        distanceSpan = 0.01f;
    }

    const float minDistance = qMin(cursorFrom, cursorTo);
    const float maxDistance = qMax(cursorFrom, cursorTo);

    float low = centerDistance - distanceSpan * 0.5f;
    float high = centerDistance + distanceSpan * 0.5f;

    if (low < minDistance) {
        const float delta = minDistance - low;
        low += delta;
        high += delta;
    }
    if (high > maxDistance) {
        const float delta = high - maxDistance;
        low -= delta;
        high -= delta;
    }

    low = qBound(minDistance, low, maxDistance);
    high = qBound(minDistance, high, maxDistance);
    if (high <= low) {
        high = qMin(maxDistance, low + 0.01f);
    }

    const bool isAscending = cursorTo >= cursorFrom;
    const float zoomFrom = isAscending ? low : high;
    const float zoomTo = isAscending ? high : low;

    QImage zoomImage(previewWidth, previewHeight, QImage::Format_ARGB32_Premultiplied);
    zoomImage.fill(Qt::transparent);
    QPainter zoomPainter(&zoomImage);

    static MiniPreviewPlot2D miniPlot;
    const int sourceLeft = clampedCenterX - srcWidth / 2;
    const bool rendered = miniPlot.render(&zoomPainter,
                                          dataset,
                                          cursor,
                                          canvas.width(),
                                          sourceLeft,
                                          srcWidth,
                                          previewWidth,
                                          previewHeight,
                                          zoomFrom,
                                          zoomTo,
                                          getThemeId(),
                                          getLowLevel(),
                                          getHighLevel(),
                                          _compensation_id);
    zoomPainter.end();

    if (!rendered) {
        return false;
    }

    painter->drawImage(targetRect, zoomImage, zoomImage.rect(), Qt::ThresholdDither);
    return true;
}

float Plot2DEchogram::getLowLevel() const
{
    return _levels.low;
}

float Plot2DEchogram::getHighLevel() const
{
    return _levels.high;
}
