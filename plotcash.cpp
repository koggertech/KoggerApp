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

void PlotCash::LineCash::setData(QVector<uint8_t> data, int resolution, int offset) {
    m_dataResol = resolution;
    m_dataOffset = offset;
    m_rawData = QVector<uint8_t>(data);
    m_isUpdated = true;
}

QImage* PlotCash::LineCash::getImage(QSize size, int range, int offset, QVector<QColor> colorMap, bool forceDraw) {
    bool need_render = forceDraw | m_isUpdated;
    m_isUpdated = false;

    if(m_cash.size() != size) {
        m_cash = QImage(size, QImage::Format_RGB555);
        need_render = true;
    }

    if(m_cashOffset != offset || m_cashResol != range) {
        need_render = true;
        m_cashOffset = offset;
        m_cashResol = range;
    }

    if(need_render) {
        float one_pixel_range = (float) (range) / (float) (m_cash.height());

        for (int i = 0; i < m_cash.height(); i++) {
            float pixel_dist = ((float) i * one_pixel_range) + (float)offset;
            float next_pixel_dist =((float) (i + 1) * one_pixel_range) + (float)offset;

            uint8_t data = rawDataRange(pixel_dist, next_pixel_dist);

            QColor cl = colorMap[data];
            for(int hl = 0; hl < m_cash.width(); hl++) {
                m_cash.setPixelColor(hl, i, cl);
            }
        }
    }

    return &m_cash;
}

uint8_t PlotCash::LineCash::rawDataRange(float start, float end) {
    int start_index = static_cast<int>((start - m_dataOffset)/(float)m_dataResol);
    int end_index = static_cast<int>((end - m_dataOffset)/(float)m_dataResol);

    if(start_index >= m_rawData.length() || start_index < 0) {
        return 0;
    }

    if(end_index > m_rawData.length()) {
        end_index = m_rawData.length();
    }

    uint8_t val = m_rawData[start_index];
    for(int i = start_index + 1; i < end_index; i++) {
        if(m_rawData[i] > val) {
            val = m_rawData[i];
        }
    }

    return val;
}


PlotCash::PlotCash() {
    setLineCount(700);
    m_colorMap.resize(256);
    for(int i = 0; i < 256; i++) {
        m_colorMap[i] = QColor::fromRgb(i,i,i);
    }
    m_cash = QImage(1, 1, QImage::Format_RGB555);
}

void PlotCash::addData(QVector<uint8_t> data, int resolution, int offset) {
    nextIndex();
    Lines[getIndex()].setData(data, resolution, offset);

    m_isDataUpdate = true;
}

QImage PlotCash::getImage(QSize size) {
    bool need_render = m_isDataUpdate;
    m_isDataUpdate = false;

    if(m_cash.size() != size) {
        m_cash = m_cash.scaled(size);
        need_render = true;
    }

    int line_w = 3;
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
            p.drawText(10, offset_y - 10, QString::number((double)range_text) + QStringLiteral(" m"));
        }

        p.drawLine(m_cash.width() - prev_line_cnt*line_w, 0, m_cash.width() - prev_line_cnt*line_w, m_cash.height());

        p.setFont(QFont("Asap", 24, QFont::Normal));
        p.drawText(10, m_cash.height() - 10, QString::number((double)(m_range*m_legendMultiply)) + QStringLiteral(" m"));

    }

    return m_cash;
}

QImage* PlotCash::getLine(int offset, QSize size) {
    return Lines[getIndex(offset)].getImage(size, m_range, m_offset, m_colorMap);
}

int PlotCash::getLineCount() {
    return Lines.length();
}

void PlotCash::setLineCount(int line_count) {
    Lines.resize(line_count);
    CurrentIndex = line_count - 1;
}
