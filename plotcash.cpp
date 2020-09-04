#include "plotcash.h"


PlotCash::PoolDataset::PoolDataset() {
    m_chartData.clear();
    m_chartData.resize(0);
    m_chartResol = 0;
    m_chartOffset = 0;
}

void PlotCash::PoolDataset::setChart(QVector<int16_t> data, int resolution, int offset) {
    m_chartResol = resolution;
    m_chartOffset = offset;
    m_chartData = QVector<int16_t>(data);
    flags.chartAvail = true;
}

void PlotCash::PoolDataset::setDist(int dist) {
    m_dist = dist;
    flags.distAvail = true;
}

void PlotCash::PoolDataset::setPosition(uint32_t date, uint32_t time, double lat, double lon) {
    m_position.date = date;
    m_position.time = time;
    m_position.lat = lat;
    m_position.lon = lon;
    flags.posAvail = true;
}

PlotCash::PlotCash() {
    m_colorMap.resize(256);
    for(int i = 0; i < 256; i++) {
        m_colorMap[i] = QColor::fromRgb(0,0,0);
    }

    QVector<QColor> coloros = { QColor::fromRgb(0, 0, 0), QColor::fromRgb(40, 0, 80), QColor::fromRgb(50, 180, 230), QColor::fromRgb(220, 255, 255)};
    QVector<int> levels = {0, 30, 130, 255};

    //    QVector<QColor> coloros = {
    //        QColor::fromRgb(0, 0, 0),
    //        QColor::fromRgb(40, 0, 80),
    //        QColor::fromRgb(0, 30, 150),
    //        QColor::fromRgb(20, 230, 30),
    //        QColor::fromRgb(255, 50, 20),
    //        QColor::fromRgb(255, 255, 255),
    //    };

    //    QVector<int> levels = {0, 30, 80, 120, 150, 255};

    setColorScheme(coloros, levels);
}

void PlotCash::addChart(QVector<int16_t> data, int resolution, int offset) {
    poolAppend();
    m_pool[poolLastIndex()].setChart(data, resolution, offset);
    m_offset = offset;
    m_range = data.length()*resolution;
    updateImage(true);
}

void PlotCash::addDist(int dist) {
    int pool_index = poolLastIndex();
    if(pool_index < 0 || m_pool[pool_index].chartAvail() == false || m_pool[pool_index].distAvail() == true) {
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
    updateImage(true);
}

void PlotCash::setDistVis(bool visible) {
    m_distSonarVis = visible;
    m_distCalcVis = visible;
    updateImage(true);
}

void PlotCash::updateImage(bool update_value) {
    bool send_update = false;

    if(update_value) {
        flags.needUpdateValueMap = true;
        send_update = true;
    }

    if(flags.needUpdateImage == false) {
        flags.needUpdateImage = true;
        send_update = true;
    }

    if(send_update) {
        emit updatedImage();
    }
}


void PlotCash::updateValueMap(int width, int height) {
    if(m_valueCash.size() != width) {
        m_valueCash.resize(width);
    }

    m_prevValueCash.chartData.resize(height);
    for(int column = 0; column < width; column++) {
        if(m_valueCash[column].chartData.size() != height) {
            m_valueCash[column].chartData.resize(height);
        }
    }

    int size_column = m_prevValueCash.chartData.size();
    int16_t* data_column = m_prevValueCash.chartData.data();
    int pool_index = poolIndex(poolLastIndex());
    if(pool_index >= 0) {
        if(m_pool[pool_index].chartAvail())  {
            m_pool[poolLastIndex()].chartTo(0, m_range, data_column, size_column);
        } else {
            memset(data_column, 0, size_column*2);
        }

        if(m_pool[pool_index].distAvail()) {
            m_prevValueCash.distData = m_pool[pool_index].distData();
        }
    }

    for(int column = 0; column < width; column++) {
        size_column = m_valueCash[column].chartData.size();
        data_column = m_valueCash[column].chartData.data();
        int avrg_cnt = 1;
        for(int avrg_index = 0; avrg_index < avrg_cnt; avrg_index++) {
            pool_index = poolIndex(poolLastIndex() - column - avrg_index - m_offsetLine);
            if(pool_index >= 0) {

                if(m_chartVis) {
                    if(m_pool[pool_index].chartAvail()) {
                        m_pool[pool_index].chartTo(0, m_range, data_column, size_column, avrg_index == 0 ? false : true );
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
            } else {
                memset(data_column, 0, size_column*2);
            }
        }

        if(m_chartVis && avrg_cnt > 1) {
            for(int i = 0; i < size_column; i++) {
                data_column[i] /= avrg_cnt;
            }
        }
    }
}

void PlotCash::updateImage(int width, int height) {
    flags.needUpdateImage = false;
    int waterfall_width = width - m_prevLineWidth;

    if(flags.needUpdateValueMap) {
        flags.needUpdateValueMap = false;
        updateValueMap(waterfall_width, height);
    }

    if(m_chartVis) {
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
            int col_value = waterfall_width - col - 1;

            int16_t* raw_col = m_valueCash[col_value].chartData.data();
            int render_height = m_valueCash[col_value].chartData.size();
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

        for(int col = 0;  col < waterfall_width; col++) {
            int col_value = waterfall_width - col - 1;

            if(col > 0) {
                int dist = m_valueCash[col_value].distData;
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
    }

    m_image = QImage((uint8_t*)m_dataImage, width, height, width*2, QImage::Format_RGB555);
}

QImage PlotCash::getImage(QSize size) {
    if(m_image.size() != size) { flags.needUpdateValueMap = true;  }

//    m_range = 25000;
    flags.needUpdateImage |= flags.needUpdateValueMap;
    if(flags.needUpdateImage) {
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
