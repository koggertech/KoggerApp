#include "plot2D_aim.h"
#include "plot2D.h"


Plot2DAim::Plot2DAim()
    : beenEpochEvent_(false),
    lineWidth_(1),
    lineColor_(255, 255, 255, 255)
{
#if defined(Q_OS_ANDROID) || defined(LINUX_ES)
    scaleFactor_ = 2;
#else
    scaleFactor_ = 1;
#endif
}

bool Plot2DAim::draw(Plot2D* parent, Dataset* dataset)
{
    auto& canvas = parent->canvas();
    auto& cursor = parent->cursor();

    if (!dataset ||
        ((cursor.mouseX < 0 || cursor.mouseY < 0) && (cursor.selectEpochIndx == -1))) {
        return false;
    }

    if (cursor.selectEpochIndx != -1) {
        auto* ep = dataset->fromIndex(cursor.selectEpochIndx);
        int offsetX = 0;
        int halfCanvas = canvas.width() / 2;
        int withoutHalf = dataset->size() - halfCanvas;

        if (cursor.selectEpochIndx >= withoutHalf) {
            offsetX = cursor.selectEpochIndx - withoutHalf;
        }

        if (const auto datasetChannels{ dataset->channelsList() }; !datasetChannels.empty()) {
            auto& firstDatasetChannels = datasetChannels.at(0);
            if (const auto chartPtr{ ep->chart(firstDatasetChannels.channelId_, firstDatasetChannels.subChannelId_) }; chartPtr) {
                const int x = canvas.width() / 2 + offsetX;
                const int y = datasetChannels.size() == 2 ? canvas.height() / 2 - canvas.height() * (chartPtr->bottomProcessing.distance / cursor.distance.range()) :
                                  canvas.height() * (chartPtr->bottomProcessing.distance / cursor.distance.range());
                cursor.setMouse(x, y);
            }
        }
    }

    QPainter* p = canvas.painter();

    QPen pen;
    pen.setWidth(lineWidth_);
    pen.setColor(lineColor_);
    p->setPen(pen);

    QFont font("Asap", 14 * scaleFactor_, QFont::Normal);
    font.setPixelSize(18 * scaleFactor_);
    p->setFont(font);

    if (cursor._tool == MouseToolNothing || beenEpochEvent_) {
        p->drawLine(0,             cursor.mouseY, canvas.width(),  cursor.mouseY);
        p->drawLine(cursor.mouseX, 0,             cursor.mouseX, canvas.height());
    }

    float canvas_height  = static_cast<float>(canvas.height());
    float value_range    = cursor.distance.to - cursor.distance.from;
    float value_scale    = float(cursor.mouseY) / canvas_height;
    float cursor_distance = value_scale * value_range + cursor.distance.from;

    p->setCompositionMode(QPainter::CompositionMode_SourceOver);

    QString distanceText = QString(QObject::tr("%1 m")).arg(cursor_distance, 0, 'g', 4);
    QString text = distanceText;

    auto [channelId, subIndx, name] = parent->getSelectedChannelId();

    if (channelId != CHANNEL_NONE) {
        text += "\n" + QObject::tr("Channel: ") + QString("%1").arg(name);
    }

    if (cursor.currentEpochIndx != -1) {
        text += "\n" + QObject::tr("Epoch: ")   + QString::number(cursor.currentEpochIndx);

        if (auto* ep = dataset->fromIndex(cursor.currentEpochIndx); ep) {
            if (auto* echogram = ep->chart(channelId, subIndx); echogram) {
                //qDebug() << "errs[" << cursor.currentEpochIndx << "]:"<< echogram->chartParameters_.errList;
                //qDebug() << "size[" << cursor.currentEpochIndx << "]:"<< echogram->amplitude.size();
                //qDebug() << "RES[" << cursor.currentEpochIndx << "]:" << echogram->resolution;

                if (!echogram->recordParameters_.isNull()) {
                    auto& recParams = echogram->recordParameters_;
                    QString boostStr = recParams.boost ? QObject::tr("ON") : QObject::tr("OFF");
                    text += "\n" + QObject::tr("Resolution, mm: ")      + QString::number(recParams.resol);
                    //text += "\n" + QObject::tr("Number of Samples: ") + QString::number(recParams.count);
                    //text += "\n" + QObject::tr("Offset of samples: ")     + QString::number(recParams.offset);
                    text += "\n" + QObject::tr("Frequency, kHz: ")      + QString::number(recParams.freq);
                    text += "\n" + QObject::tr("Pulse count: ")         + QString::number(recParams.pulse);
                    text += "\n" + QObject::tr("Booster: ")             + boostStr;
                    text += "\n" + QObject::tr("Speed of sound, m/s: ") + QString::number(recParams.soundSpeed / 1000);
                }
            }
        }
    }

    QRect textRect = p->fontMetrics().boundingRect(QRect(0, 0, 9999, 9999), Qt::AlignLeft | Qt::AlignTop, text);

    int xShift    = 50 * scaleFactor_;
    int yShift    = 40 * scaleFactor_;
    int xCheck    = xShift + 15 * scaleFactor_;
    int yCheck    = yShift + 15 * scaleFactor_;

    bool onTheRight = (p->window().width() - cursor.mouseX - xCheck) < textRect.width();

    int spaceBelow = cursor.mouseY;
    bool placeAbove = false;

    int neededSpaceBelow = textRect.height() + yCheck;
    if (spaceBelow < neededSpaceBelow) {
        placeAbove = true;
    }

    QPoint shiftedPoint;
    if (!placeAbove) {
        shiftedPoint = onTheRight
                           ? QPoint(cursor.mouseX - xShift - textRect.width(),
                                    cursor.mouseY - yShift - textRect.height())
                           : QPoint(cursor.mouseX + xShift,
                                    cursor.mouseY - yShift - textRect.height());
    } else {
        shiftedPoint = onTheRight
                           ? QPoint(cursor.mouseX - xShift - textRect.width(),
                                    cursor.mouseY + yShift)
                           : QPoint(cursor.mouseX + xShift,
                                    cursor.mouseY + yShift);
    }

    textRect.moveTopLeft(shiftedPoint);

    // back
    p->setPen(Qt::NoPen);
    p->setBrush(QColor(45, 45, 45));
    int margin = 5 * scaleFactor_;
    QRect backgroundRect = textRect.adjusted(-margin, -margin, margin, margin);
    p->drawRect(backgroundRect);

    // text
    p->setPen(QColor(255,255,255));
    p->drawText(textRect, Qt::AlignLeft | Qt::AlignTop, text);

    return true;
}

void Plot2DAim::setEpochEventState(bool state)
{
    beenEpochEvent_ = state;
}
