#include "plotcash.h"


PoolDataset::PoolDataset() {
    m_chartData.clear();
    m_chartData.resize(0);
    m_chartResol = 0;
    m_chartOffset = 0;
    flags.distAvail = false;
}

void PoolDataset::setEvent(int timestamp, int id) {
    _eventTimestamp = timestamp;
    _eventId = id;
    flags.eventAvail = true;
}

void PoolDataset::setChart(QVector<int16_t> data, int resolution, int offset) {
    m_chartResol = resolution;
    m_chartOffset = offset;
    m_chartData = data;
    flags.chartAvail = true;
}

void PoolDataset::setDist(int dist) {
    m_dist = dist;
    flags.distAvail = true;
}

void PoolDataset::setPosition(uint32_t date, uint32_t time, double lat, double lon) {
    m_position.date = date;
    m_position.time = time;
    m_position.lat = lat;
    m_position.lon = lon;
    flags.posAvail = true;
}

void PoolDataset::setEncoders(int16_t enc1, int16_t enc2, int16_t enc3, int16_t enc4, int16_t enc5, int16_t enc6) {
    encoder.e1 = enc1;
    encoder.e2 = enc2;
    encoder.e3 = enc3;
    encoder.e4 = enc4;
    encoder.e5 = enc5;
    encoder.e6 = enc6;
    encoder.valid = true;
}

PlotCash::PlotCash() {
    resetDataset();

    m_colorMap.resize(256);
    for(int i = 0; i < 256; i++) {
        m_colorMap[i] = QColor::fromRgb(0,0,0);
    }

    setThemeId(0);
}

void PlotCash::addEvent(int timestamp, int id) {
    lastEventTimestamp = timestamp;
    lastEventId = id;

    poolAppend();
    m_pool[poolLastIndex()].setEvent(timestamp, id);
}

void PlotCash::addTimestamp(int timestamp) {
}

void PlotCash::addChart(QVector<int16_t> data, int resolution, int offset) {
    int pool_index = poolLastIndex();

    if(pool_index < 0 || m_pool[pool_index].eventAvail() == false || m_pool[pool_index].chartAvail() == true) {
        poolAppend();
        pool_index = poolLastIndex();
    }

//    poolAppend();
    m_pool[poolLastIndex()].setChart(data, resolution, offset);

    if(m_distProcessingVis) {
        m_pool[poolLastIndex()].doDistProccesing();
    }

    m_offset = offset;
    m_range = data.length()*resolution;
    updateImage(true);
}

void PlotCash::addDist(int dist) {
    int pool_index = poolLastIndex();
    if(pool_index < 0 || (m_pool[pool_index].eventAvail() == false && m_pool[pool_index].chartAvail() == false) || m_pool[pool_index].distAvail() == true) {
        poolAppend();
        pool_index = poolLastIndex();
    }

    m_pool[poolLastIndex()].setDist(dist);
    updateImage(true);
}

void PlotCash::addPosition(uint32_t date, uint32_t time, double lat, double lon) {
    int pool_index = poolLastIndex();
    if(pool_index < 0) {
        poolAppend();
        pool_index = poolLastIndex();
    }
    m_pool[poolLastIndex()].setPosition(date, time, lat, lon);
}

void PlotCash::setColorScheme(QVector<QColor> coloros, QVector<int> levels) {
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
            m_colorMap[i_level] = QColor::fromRgb(red, green, blue);
            m_colorHashMap[i_level] = ((red / 8) << 10) | ((green / 8) << 5) | ((blue / 8));
            i_level++;
        }
    }

    m_colorDist = ((220 / 8) << 10) | ((50 / 8) << 5) | ((0 / 8));
    updateImage();
}

void PlotCash::setStartLevel(int level) {
    m_startLevel = level;
    updateImage();
}

void PlotCash::setStopLevel(int level) {
    m_stopLevel = level;
    updateImage();
}

void PlotCash::setTimelinePosition(double position) {
    int m_lineVisibleCount = m_image.width();
    if(m_lineVisibleCount > poolSize()) {
        m_lineVisibleCount = poolSize();
    }
    m_offsetLine = (int)(position*(double)(poolSize() - m_lineVisibleCount));
    updateImage(true);
}

void PlotCash::setChartVis(bool visible) {
    m_chartVis = visible;
    resetValue();
    updateImage(true);
}

void PlotCash::setOscVis(bool visible) {
    m_oscVis = visible;
    resetValue();
    updateImage(true);
}

void PlotCash::setDistVis(bool visible) {
    m_distSonarVis = visible;
    resetValue();
    updateImage(true);
}

void PlotCash::setDistProcVis(bool visible) {
    m_distProcessingVis = visible;
    if(visible) {
        doDistProcessing(true);
    }

    resetValue();
    updateImage(true);
}

void PlotCash::updateImage(bool update_value) {
    bool send_update = false;

    if(update_value) {
        renderValue();
        send_update = true;
    }

    if(flags.renderImage == false) {
        flags.renderImage = true;
        send_update = true;
    }

    if(send_update) {
        emit updatedImage();
    }
}

void PlotCash::renderValue() {
    flags.renderValue = true;
}

void PlotCash::resetValue() {
    flags.resetValue = true;
}

void PlotCash::resetDataset() {
    m_pool.clear();
    resetValue();
    m_valueCash.clear();
}

void PlotCash::doDistProcessing(bool processing) {
    int pool_size = poolSize();
    for(int i = 0; i < pool_size; i++) {
        PoolDataset* dataset = fromPool(i);
        if(processing) {
            dataset->doDistProccesing();
        } else {
            dataset->resetDistProccesing();
        }
    }
}

void PlotCash::setThemeId(int theme_id) {
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
    updateImage();
}

void PlotCash::updateValueMap(int width, int height) {
    if(m_valueCash.size() != width) {
        m_valueCash.resize(width);
    }

    m_prevValueCash.chartData.resize(height);
    for(int column = 0; column < width; column++) {
        m_valueCash[column].poolIndexUpdate = false;
        if(m_valueCash[column].chartData.size() != height) {
            m_valueCash[column].chartData.resize(height);
            m_valueCash[column].poolIndexUpdate = true;
        }
    }

    int pool_last_index = poolLastIndex();

    int size_column = m_prevValueCash.chartData.size();
    int16_t* data_column = m_prevValueCash.chartData.data();

    int pool_index = poolIndex(pool_last_index);
    if(pool_index >= 0) {
        if(m_pool[pool_index].chartAvail())  {
            m_pool[pool_last_index].chartTo(0, m_range, data_column, size_column);
        } else {
            memset(data_column, 0, size_column*2);
        }

        if(m_pool[pool_index].distAvail()) {
            m_prevValueCash.distData = m_pool[pool_index].distData();
        }
    }

    int pool_offset_index = pool_last_index - m_offsetLine - width;
    m_valueCashStart = qAbs(pool_offset_index % width);

    bool force_reset = flags.resetValue = true;
    flags.resetValue = false;

    for(int column = 0; column < width; column++) {
        int val_col = (m_valueCashStart + column);
        if(val_col >= width) {
            val_col -= width;
        }

        int pool_ind = poolIndex(pool_offset_index + column);
        if(m_valueCash[val_col].poolIndex != pool_ind || force_reset) {
            m_valueCash[val_col].poolIndex = pool_ind;
            m_valueCash[val_col].poolIndexUpdate = true;
        }
    }

    for(int column = 0; column < width; column++) {
        if(!m_valueCash[column].poolIndexUpdate) {
            continue;
        }
        m_valueCash[column].poolIndexUpdate = false;

        size_column = m_valueCash[column].chartData.size();
        data_column = m_valueCash[column].chartData.data();

        int pool_index = m_valueCash[column].poolIndex;

        if(pool_index >= 0) {
            if(m_chartVis) {
                if(m_pool[pool_index].chartAvail()) {
                    m_pool[pool_index].chartTo(0, m_range, data_column, size_column);
                } else {
                    memset(data_column, 0, size_column*2);
                }
            }

            if(m_distSonarVis) {
                if(m_pool[pool_index].distAvail()) {
                    m_valueCash[column].distData = m_pool[pool_index].distData();
                } else {
                    m_valueCash[column].distData = -1;
                }
            }

            if(m_distProcessingVis) {
                if(m_pool[pool_index].distProccesingAvail()) {
                    m_valueCash[column].processingDistData = m_pool[pool_index].distProccesing();
                } else {
                    m_valueCash[column].processingDistData = -1;
                }
            }
        } else {
            memset(data_column, 0, size_column*2);
            m_valueCash[column].distData = -1;
            m_valueCash[column].processingDistData = -1;
        }
    }
}

void PlotCash::updateImage(int width, int height) {
    flags.renderImage = false;
    int waterfall_width = width - m_prevLineWidth;

    if(flags.renderValue) {
        flags.renderValue = false;
        updateValueMap(waterfall_width, height);
    }

    if(m_chartVis) {
        memset(m_dataImage, 0, width*height*2);

        int level_range = m_stopLevel - m_startLevel;
        int index_offset = (int)((float)m_startLevel*2.5f);
        float index_map_scale = 0;
        if(level_range > 0) {
            index_map_scale = (float)(m_colorMap.length() - 1)/((float)(m_stopLevel - m_startLevel)*2.5f);
        } else {
            index_map_scale = 10000;
        }

        int16_t* raw_col = m_prevValueCash.chartData.data();
        for (int row = 0; row < height; row++) {
            int16_t index_map = (float)(raw_col[row] - index_offset)*index_map_scale;
            if(index_map < 0) { index_map = 0;
            } else if(index_map > 255) { index_map = 255; }

            for(int col = 1;  col < m_prevLineWidth; col++) {
                m_dataImage[width - col + row*width] = m_colorHashMap[index_map];
            }
        }

        for(int col = 0;  col < waterfall_width; col++) {
            int val_col = (m_valueCashStart + col);
            if(val_col >= waterfall_width) {
                val_col -= waterfall_width;
            }

            int16_t* raw_col = m_valueCash[val_col].chartData.data();
            int render_height = m_valueCash[val_col].chartData.size();
            if(render_height > height) {
                render_height = height;
            }

            for (int row = 0; row < render_height; row++) {
                int32_t index_map = (float)(raw_col[row] - index_offset)*index_map_scale;
                if(index_map < 0) { index_map = 0;
                } else if(index_map > 255) { index_map = 255; }
                m_dataImage[col + row*width] = m_colorHashMap[index_map];
            }
        }
    } else {
        memset(m_dataImage, 0, width*height*2);
    }

    if(m_oscVis) {
        int16_t* raw_col = m_prevValueCash.chartData.data();

        float scale_scope_w = (float)height / (float)(waterfall_width - 100);
        QImage tmp_img = QImage((uint8_t*)m_dataImage, width, height, width*2, QImage::Format_RGB555);
        QPainter p(&tmp_img);
        QPen pen;
        pen.setWidth(2);
        pen.setColor(QColor::fromRgb(70, 255, 0));
        p.setPen(pen);

        float scale_y = (float)height*(1.0f/256.0f);
        float scale_x = (float)(waterfall_width - 104)/height;

        int last_offset_y = (float)raw_col[0]*scale_y;
        int last_col = 100;

        for (int row = 1; row < height; row++) {
            int offset_y = (float)raw_col[row]*scale_y;
            int offset_x = (float)row*scale_x + 100;


            if(last_offset_y != offset_y) {
                p.drawLine(last_col, height - last_offset_y, offset_x, height - offset_y);
                last_col = offset_x;
            }

            last_offset_y = offset_y;
        }
    }

    if(m_distSonarVis) {
        int dist = m_prevValueCash.distData;
        if(dist >= 0) {
            int index_dist = (int)((float)dist/(float)m_range*(float)height);
            if(index_dist < 0) {
                index_dist = 0;
            } else if(index_dist > height - 2) {
                index_dist = height - 2;
            }

            for(int col = 1;  col < m_prevLineWidth; col++) {
                m_dataImage[width - col + index_dist*width] = m_colorDist;
                m_dataImage[width - col + (index_dist + 1)*width] = m_colorDist;
            }
        }

        for(int col = 1;  col < waterfall_width; col++) {
            int val_col = (m_valueCashStart + col);
            if(val_col >= waterfall_width) {
                val_col -= waterfall_width;
            }

            int dist = m_valueCash[val_col].distData;
            if(dist >= 0) {
                int index_dist = (int)((float)dist/(float)m_range*(float)height);
                if(index_dist < 0) {
                    index_dist = 0;
                } else if(index_dist > height - 2) {
                    index_dist = height - 2;
                }

                m_dataImage[col - 1 + index_dist*width] = m_colorDist;
                m_dataImage[col - 1 + (index_dist + 1)*width] = m_colorDist;
                m_dataImage[col + index_dist*width] = m_colorDist;
                m_dataImage[col + (index_dist + 1)*width] = m_colorDist;
            }
        }
    }

    if(m_distProcessingVis) {

        for(int col = 1;  col < waterfall_width; col++) {
            int val_col = (m_valueCashStart + col);
            if(val_col >= waterfall_width) {
                val_col -= waterfall_width;
            }

            int dist = m_valueCash[val_col].processingDistData;
            if(dist >= 0) {
                int index_dist = (int)((float)dist/(float)m_range*(float)height);
                if(index_dist < 0) {
                    index_dist = 0;
                } else if(index_dist > height - 2) {
                    index_dist = height - 2;
                }

                m_dataImage[col - 1 + index_dist*width] = m_colorDistProc;
                m_dataImage[col - 1 + (index_dist + 1)*width] = m_colorDistProc;
                m_dataImage[col + index_dist*width] = m_colorDistProc;
                m_dataImage[col + (index_dist + 1)*width] = m_colorDistProc;
            }
        }
    }

    m_image = QImage((uint8_t*)m_dataImage, width, height, width*2, QImage::Format_RGB555);
}

QImage PlotCash::getImage(QSize size) {
    if(m_image.size() != size) { renderValue();  }

    flags.renderImage |= flags.renderValue;
    if(flags.renderImage) {
        updateImage(size.width(), size.height());

        QPainter p(&m_image);
        p.setPen(QColor::fromRgb(100, 100, 100));
        p.setFont(QFont("Asap", 13, QFont::Normal));

        int nbr_hor_div = m_verticalGridNum;
        for (int i = 1; i < nbr_hor_div; i++) {
            int offset_y = m_image.height()*i/nbr_hor_div;
            p.drawLine(0, offset_y, m_image.width(), offset_y);

            float range_text = (float)(m_range*i/nbr_hor_div + m_offset)*m_legendMultiply;
            p.drawText(m_image.width() - m_prevLineWidth - 70, offset_y - 10, QString::number((double)range_text) + QStringLiteral(" m"));
        }

        p.drawLine(m_image.width() - m_prevLineWidth, 0, m_image.width() - m_prevLineWidth, m_image.height());
        p.setFont(QFont("Asap", 24, QFont::Normal));
        p.drawText(m_image.width() - m_prevLineWidth - 80, m_image.height() - 10, QString::number((double)((m_range + m_offset)*m_legendMultiply)) + QStringLiteral(" m"));

        if(m_distSonarVis) {
            p.setPen(QColor::fromRgb(200, 50, 0));
            int disp_dist = m_prevValueCash.distData;
            if(disp_dist >= 0) {
                p.drawText(m_image.width() - m_prevLineWidth - 80, m_image.height() - 60, QString::number((double)(m_prevValueCash.distData)*0.001) + QStringLiteral(" m"));
            }
        }
    }

    return m_image;
}




int PlotCash::poolSize() {
    return m_pool.length();
}
