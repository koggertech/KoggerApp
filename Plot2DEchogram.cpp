#include "Plot2D.h"


Plot2DEchogram::Plot2DEchogram() {
    setThemeId(ClassicTheme);
    setLevels(10, 100);
}

void Plot2DEchogram::setLowLevel(float low) {
    _levels.low = low;
}

void Plot2DEchogram::setHightLevel(float high) {
    _levels.high = high;
}

void Plot2DEchogram::setLevels(float low, float high) {
    _levels.low = low;
    _levels.high = high;
}

void Plot2DEchogram::setColorScheme(QVector<QColor> coloros, QVector<int> levels) {
    if(coloros.length() != levels.length()) { return; }

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
            i_level++;
        }
    }
}

void Plot2DEchogram::setThemeId(int theme_id) {
    QVector<QColor> coloros;
    QVector<int> levels;

    if(theme_id == ClassicTheme) {
        coloros = { QColor::fromRgb(0, 0, 0), QColor::fromRgb(20, 5, 80), QColor::fromRgb(50, 180, 230), QColor::fromRgb(220, 255, 255)};
        levels = {0, 30, 130, 255};
    } else if(theme_id == SepiaTheme) {
        coloros = { QColor::fromRgb(0, 0, 0), QColor::fromRgb(50, 50, 10), QColor::fromRgb(230, 200, 100), QColor::fromRgb(255, 255, 220)};
        levels = {0, 30, 130, 255};
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
    }

    setColorScheme(coloros, levels);
}

void Plot2DEchogram::resetCash() {
    _cashFlags.resetCash = true;
}

int Plot2DEchogram::updateCash(Dataset* dataset, DatasetCursor cursor, int width, int height) {
    if(_cash.size() != width) {
        _cash.resize(width);
        resetCash();
    }

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

    if(to >= 0) {
        range1 = 0 - from;
        from1 = 0;
        to1 = -from;

        if(from >= 0) { from2 = from; }
        else { from2 = 0; }
        to2 = to;
    } else {
        range1 = to - from;
        from1 = -to;
        to1 = -from;
    }


    int wrap_start_pos = qAbs(cursor.getIndex(0) % width);
    _cashPosition = wrap_start_pos;
    for(int column = 0; column < width; column++) {
        if(_cash[column].data.size() != height) {
            _cash[column].state = CashLine::CashStateNotValid;
            _cash[column].data.resize(height);
            _cash[column].data.fill(0);
            _cash[column].poolIndex = -1;
            _cash[column].state = CashLine::CashStateEraced;
        }

        int cursor_pos = column - wrap_start_pos;
        if(column < wrap_start_pos) {
            cursor_pos += width;
        }

        int pool_index = cursor.getIndex(cursor_pos);
        int pool_index_safe = dataset->validIndex(pool_index);
        if(pool_index_safe >= 0) {
            const int cash_index = _cash[column].poolIndex;
            if(is_cash_notvalid || pool_index_safe != cash_index) {
                _cash[column].state = CashLine::CashStateNotValid;
                _cash[column].poolIndex = pool_index_safe;

                Epoch* datasource = dataset->fromIndex(pool_index_safe);
                if(datasource != NULL) {
                    int16_t* cash_data = _cash[column].data.data();
                    int16_t cash_data_size = _cash[column].data.size();

                    if(cursor.channel2 == CHANNEL_NONE) {
                        datasource->chartTo(cursor.channel1, from, to, cash_data, cash_data_size, 1);
                    } else {
                        int cash_data_size_part1 = cash_data_size*(range1/fullrange);

                        if(cash_data_size_part1 > 0) {
                            datasource->chartTo(cursor.channel1, from1, to1, cash_data, cash_data_size_part1, 1, true);
                        }

                        if(cash_data_size_part1 < 0) {
                            cash_data_size_part1 = 0;
                        }

                        const int cash_data_size_part2 = cash_data_size - cash_data_size_part1;
                        if(cash_data_size_part2 > 0) {
                            datasource->chartTo(cursor.channel2, from2, to2, &cash_data[cash_data_size_part1], cash_data_size_part2, 1, false);
                        }
                    }

                    _cash[column].state = CashLine::CashStateValid;
                } else {
                    _cash[column].data.fill(0);
                    _cash[column].poolIndex = -1;
                    _cash[column].state = CashLine::CashStateEraced;
                }
            }
        } else {
            _cash[column].state = CashLine::CashStateNotValid;
            _cash[column].data.fill(0);
            _cash[column].poolIndex = -1;
            _cash[column].state = CashLine::CashStateEraced;
        }
    }

    _lastCursor = cursor;
    _lastWidth = width;
    _lastHeight = height;

    return wrap_start_pos;
}

bool Plot2DEchogram::draw(Canvas& canvas, Dataset* dataset, DatasetCursor cursor) {
    if(isVisible() && dataset != nullptr && cursor.distance.isValid()) {
        uint16_t* image_data = canvas.data();
        const int cash_width = canvas.width();
        const int image_width = canvas.width();
        const int image_height = canvas.height();
        const int lowLevel = _levels.low;
        const int hightLevel = _levels.high;

        const int cash_position = updateCash(dataset, cursor, cash_width, image_height);

        int level_range = hightLevel - lowLevel;
        int index_offset = (int)((float)lowLevel*2.5f);
        float index_map_scale = 0;
        if(level_range > 0) {
            index_map_scale = (float)(256 - 1)/((float)(hightLevel - lowLevel)*2.5f);
        } else {
            index_map_scale = 10000;
        }

        for(int image_col = 0;  image_col < cash_width; image_col++) {
            int cash_col = cash_position + image_col;
            if(cash_col >= cash_width) {
                cash_col -= cash_width;
            }

            int16_t* cashdata = _cash[cash_col].data.data();
            for (int image_row = 0; image_row < image_height; image_row++) {
                int index_map = ((float)(cashdata[image_row] - index_offset)*index_map_scale);
                if(index_map < 0) { index_map = 0; }
                else if(index_map > 255) { index_map = 255; }

                image_data[image_row*image_width + image_col] = _colorHashMap[index_map];
            }
        }
    }

    return true;
}


