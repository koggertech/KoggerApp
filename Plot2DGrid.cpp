#include "Plot2D.h"


Plot2DGrid::Plot2DGrid() {
}


bool Plot2DGrid::draw(Canvas& canvas, Dataset* dataset, DatasetCursor cursor) {
    if(isVisible()) {
        QPen pen;
        pen.setWidth(_lineWidth);
        pen.setColor(_lineColor);

        QPainter* p = canvas.painter();
        p->setPen(pen);
        p->setFont(QFont("Asap", 14, QFont::Normal));

        const int image_height = canvas.height();
        const int image_width = canvas.width();
        const int lines_count = _lines;
        for (int i = 1; i < lines_count; i++) {
            int pos_y = i*image_height/lines_count;

            p->drawLine(0, pos_y, image_width, pos_y);
        }

        int text_offset = 10;

        if(cursor.distance.isValid()) {
            const float distance_from = cursor.distance.from;
            const float distance_to = cursor.distance.to;
            const float distance_range = distance_to - distance_from;

            for (int i = 1; i < lines_count; i++) {
                float range_text = distance_range*i/lines_count + distance_from;
                int pos_y = i*image_height/lines_count;

                p->drawText(image_width - 100, pos_y - text_offset, QString::number(range_text) + QStringLiteral(" m"));
            }

            p->setFont(QFont("Asap", 26, QFont::Normal));
            QString range_text = QString::number(cursor.distance.to) + QStringLiteral(" m");
            p->drawText(image_width - 50 - range_text.count()*25, image_height - 10, range_text);

            text_offset -= 170;
        }

        if(_rangeFinderLastVisible && cursor.distance.isValid()) {
            Epoch* last_epoch = dataset->last();
            Epoch* lastlast_epoch = dataset->lastlast();

            float distance = NAN;

            if(last_epoch != NULL && isfinite(last_epoch->rangeFinder())) {
                distance = last_epoch->rangeFinder();
            } else if(lastlast_epoch != NULL && isfinite(lastlast_epoch->rangeFinder())) {
                distance = lastlast_epoch->rangeFinder();
            }

            if(isfinite(distance)) {
                pen.setColor(QColor(250, 100, 0));
                p->setPen(pen);

                p->setFont(QFont("Asap", 40, QFont::Normal));
                QString range_text = QString::number(round(distance*100.0)/100.0) + QStringLiteral(" m");
                p->drawText(image_width/2 - range_text.count()*32, image_height - 15, range_text);

                text_offset -= 140;
            }
        }



        if(cursor.attitude.isValid()) {
            const float attitude_from = cursor.attitude.from;
            const float attitude_to = cursor.attitude.to;
            const float attitude_range = attitude_to - attitude_from;

            p->setFont(QFont("Asap", 14, QFont::Normal));

            for (int i = 1; i < lines_count; i++) {
                float attitude_text = attitude_range*i/lines_count + attitude_from;
                int pos_y = i*image_height/lines_count;

                p->drawText(image_width - 150, pos_y - 10, QString::number(attitude_text) + QStringLiteral(" °"));
            }

            text_offset -= 140;
        }

        if(_velocityVisible && cursor.velocity.isValid()) {
            const float velocity_from = cursor.velocity.from;
            const float velocity_to = cursor.velocity.to;
            const float velocity_range = velocity_to - velocity_from;

            p->setFont(QFont("Asap", 14, QFont::Normal));

            for (int i = 1; i < lines_count; i++) {
                float attitude_text = velocity_range*i/lines_count + velocity_from;
                int pos_y = i*image_height/lines_count;

                p->drawText(image_width - 180, pos_y - 10, QString::number(attitude_text) + QStringLiteral(" m/s"));
            }
        }

    }

    return true;
}
