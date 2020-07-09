#include "plotcash.h"


PlotCash::LineCash::LineCash() {
    m_rawData.clear();
    m_rawData.resize(0);
    m_dataResol = 0;
    m_dataOffset = 0;
    m_cashResol = 0;
    m_cashOffset = 0;
    m_cash = QImage(1, 1, QImage::Format_RGB555);
}

void PlotCash::LineCash::setData(QVector<int16_t> data, int resolution, int offset) {
    m_dataResol = resolution;
    m_dataOffset = offset;
    m_rawData = QVector<int16_t>(data);
    m_isUpdated = true;
}

QImage* PlotCash::LineCash::getImage(QSize size, int range, int offset, QVector<QColor> colorMap, int startLevel, int stopLevel, bool forceDraw) {
    bool need_render = forceDraw | m_isUpdated;
    m_isUpdated = false;

    if(m_cash.size() != size) {
        m_cash = QImage(size, QImage::Format_RGB555);
        m_cash.fill(colorMap[0]);
        need_render = true;
    }

    if(m_cashOffset != offset || m_cashResol != range) {
        m_cashOffset = offset;
        m_cashResol = range;
        need_render = true;
    }

    if(m_startLevel != startLevel || m_stopLevel != stopLevel) {
        m_startLevel = startLevel;
        m_stopLevel = stopLevel;
        need_render = true;
    }

    if(need_render) {
        int cash_h = m_cash.height();
        int cash_w = m_cash.width();
        int raw_len = m_rawData.length();

        if(raw_len > 0) {

            float one_pixel_range = (float) (range) / (float) (cash_h);
            int16_t* data_arr = m_rawData.data();

            float offset_f = (float)offset;
            float index_map_scale = 0;

            int level_range = stopLevel - startLevel;
            if(level_range > 0) {
                index_map_scale = (colorMap.length() - 1)/(float)(stopLevel - startLevel);
            } else {
                index_map_scale = 10000;
            }

            for (int i = 0; i < cash_h; i++) {
                float pixel_dist = ((float) i * one_pixel_range) + offset_f;
                float next_pixel_dist =((float) (i + 1) * one_pixel_range) + offset_f;

                int data = rawDataRange(data_arr, raw_len, pixel_dist, next_pixel_dist);

                int index_map = ((float)data*0.4f - (float)startLevel)*index_map_scale;

                index_map = qMin(qMax((int)index_map, 0), 255);

                QColor cl = colorMap[index_map];

                for(int hl = 0; hl < cash_w; hl++) {
                    m_cash.setPixelColor(hl, i, cl);
                }
            }
        }
    }

    return &m_cash;
}


PlotCash::PlotCash() {
    setLineCount(5000);
    m_colorMap.resize(256);

    for(int i = 0; i < 256; i++) {
        m_colorMap[i] = QColor::fromRgb(0,0,0);
    }

    QVector<QColor> coloros = { QColor::fromRgb(0, 0, 0), QColor::fromRgb(40, 0, 80), QColor::fromRgb(50, 180, 230), QColor::fromRgb(220, 255, 255)};
    QVector<int> levels = {0, 30, 130, 255};

    setColorScheme(coloros, levels);

    m_cash = QImage(1, 1, QImage::Format_RGB555);
}

void PlotCash::addData(QVector<int16_t> data, int resolution, int offset) {
    nextIndex();
    Lines[getIndex()].setData(data, resolution, offset);
    m_offset = offset;
    m_range = data.length()*resolution;
    m_isDataUpdate = true;
}

void PlotCash::addDist(int dist) {

}

void PlotCash::setColorScheme(QVector<QColor> coloros, QVector<int> levels) {
    if(coloros.length() != levels.length()) {
        return;
    }

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
            i_level++;
        }
    }

    m_isDataUpdate = true;
}

void PlotCash::setStartLevel(int level) {
    m_startLevel = level;
    m_isDataUpdate = true;
}

void PlotCash::setStopLevel(int level) {
    m_stopLevel = level;
    m_isDataUpdate = true;
}

QImage PlotCash::getImage(QSize size) {
    bool need_render = m_isDataUpdate;
    m_isDataUpdate = false;

    if(m_cash.size() != size) {
        m_cash = m_cash.scaled(size);
        need_render = true;
    }

    int line_w = 4;
    if(need_render) {
        QPainter p(&m_cash);

        int line_disp_cnt = size.width()/line_w + 1;
        if(line_disp_cnt > getLineCount()) {
            line_disp_cnt = getLineCount();
            line_w = size.width() / line_disp_cnt;
        }

        int prev_line_cnt = 10;
        for(int i = 0; i < prev_line_cnt; i++) {
            p.drawImage(QRect(size.width() - line_w*(i + 1), 0, line_w, size.height()), *getLine(0, {line_w, size.height()}));
        }

        for(int i = 0; i < line_disp_cnt; i++) {
            p.drawImage(QRect(size.width() - line_w*(i + 1 + prev_line_cnt), 0, line_w, size.height()), *getLine(i, {line_w, size.height()}));
        }

        p.setPen(QColor::fromRgb(100, 100, 100));
        p.setFont(QFont("Asap", 13, QFont::Normal));

        int nbr_hor_div = m_hGrid;

        for (int i = 1; i < nbr_hor_div; i++) {
            int offset_y = m_cash.height()*i/nbr_hor_div;
            p.drawLine(0, offset_y, m_cash.width(), offset_y);

            float range_text = (float)(m_range*i/nbr_hor_div + m_offset)*m_legendMultiply;
//            p.drawText(10, offset_y - 10, QString::number((double)range_text) + QStringLiteral(" m"));
        }

        p.drawLine(m_cash.width() - prev_line_cnt*line_w, 0, m_cash.width() - prev_line_cnt*line_w, m_cash.height());

        p.setFont(QFont("Asap", 24, QFont::Normal));
        p.drawText(10, m_cash.height() - 10, QString::number((double)((m_range + m_offset)*m_legendMultiply)) + QStringLiteral(" m"));

    }

    return m_cash;
}

QImage* PlotCash::getLine(int offset, QSize size) {
    return Lines[getIndex(offset)].getImage(size, m_range, m_offset, m_colorMap, m_startLevel, m_stopLevel);
}



int PlotCash::getLineCount() {
    return Lines.length();
}

void PlotCash::setLineCount(int line_count) {
    Lines.resize(line_count);
    CurrentIndex = line_count - 1;
}
