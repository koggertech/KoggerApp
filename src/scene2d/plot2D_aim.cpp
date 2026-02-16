#include "plot2D_aim.h"
#include "plot2D.h"
#include <cmath>


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

        if (ep) {
            ChannelId channelId = cursor.channel1;
            uint8_t subChannelId = cursor.subChannel1;

            const auto datasetChannels = dataset->channelsList();
            if (!channelId.isValid() && !datasetChannels.empty()) {
                channelId = datasetChannels.at(0).channelId_;
                subChannelId = datasetChannels.at(0).subChannelId_;
            }

            auto* chartPtr = ep->chart(channelId, subChannelId);
            if (!chartPtr && !datasetChannels.empty()) {
                // Fallback to legacy behavior if selected channel has no chart in this epoch.
                chartPtr = ep->chart(datasetChannels.at(0).channelId_, datasetChannels.at(0).subChannelId_);
            }

            if (chartPtr) {
                const int x = canvas.width() / 2 + offsetX;
                float bottomDistance = chartPtr->bottomProcessing.distance;
                if (!std::isfinite(bottomDistance)) {
                    bottomDistance = 0.0f;
                }

                const float distanceRange = cursor.distance.range();
                int y = (cursor.channel2 != CHANNEL_NONE) ? canvas.height() / 2 : 0;
                if (std::isfinite(distanceRange) && std::abs(distanceRange) > 1e-6f) {
                    const float yFloat = (cursor.channel2 != CHANNEL_NONE)
                        ? static_cast<float>(canvas.height()) * 0.5f
                            - static_cast<float>(canvas.height()) * (bottomDistance / distanceRange)
                        : static_cast<float>(canvas.height()) * (bottomDistance / distanceRange);
                    y = qRound(yFloat);
                }

                y = qBound(0, y, qMax(0, canvas.height() - 1));
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

    const int xShift = 50 * scaleFactor_;
    const int yShift = 40 * scaleFactor_;
    const int textMargin = 5 * scaleFactor_;
    const QRect textBackgroundLocal = textRect.adjusted(-textMargin, -textMargin, textMargin, textMargin);
    const int textBoxWidth = textBackgroundLocal.width();
    const int textBoxHeight = textBackgroundLocal.height();

    const bool loupeEnabled = parent->getLoupeVisible();
    const int loupeSize = qBound(1, parent->getLoupeSize(), 3);
    const int loupeZoom = qBound(1, parent->getLoupeZoom(), 3);
    const float loupeSizeMultiplier = (loupeSize == 1) ? 1.0f : ((loupeSize == 2) ? 1.5f : 2.25f);
    const float loupeZoomMultiplier = (loupeZoom == 1) ? 1.0f : ((loupeZoom == 2) ? 1.5f : 2.25f);
    const int previewFrameMargin = 10 * scaleFactor_;
    const int maxPreviewSize = qMin(canvas.width() - previewFrameMargin * 2, canvas.height() - previewFrameMargin * 2);
    const bool hasPreview = loupeEnabled && maxPreviewSize > 30 * scaleFactor_;
    const int previewSize = hasPreview ? qMin(static_cast<int>(180.0f * scaleFactor_ * loupeSizeMultiplier), maxPreviewSize) : 0;
    const int previewSourceBaseSize = hasPreview ? qMax(8, previewSize / 4) : 0;
    const int previewSourceSize = hasPreview
        ? qMax(4, static_cast<int>(static_cast<float>(previewSourceBaseSize) / loupeZoomMultiplier))
        : 0;
    const int previewGap = hasPreview ? 14 * scaleFactor_ : 0;

    const int layoutWidth = textBoxWidth + (hasPreview ? previewGap + previewSize : 0);
    const int layoutHeight = qMax(textBoxHeight, hasPreview ? previewSize : 0);
    const int layoutMargin = previewFrameMargin;

    const int rightStartX = cursor.mouseX + xShift;
    const int leftEndX = cursor.mouseX - xShift;
    const int availableRight = canvas.width() - layoutMargin - rightStartX;
    const int availableLeft = leftEndX - layoutMargin;

    bool placeToRight = true;
    if (availableRight < layoutWidth && availableLeft >= layoutWidth) {
        placeToRight = false;
    }
    else if (availableRight < layoutWidth && availableLeft < layoutWidth) {
        placeToRight = availableRight >= availableLeft;
    }

    const int aboveY = cursor.mouseY - yShift - layoutHeight;
    const int belowY = cursor.mouseY + yShift;
    const bool canPlaceAbove = aboveY >= layoutMargin;
    const bool canPlaceBelow = belowY + layoutHeight <= canvas.height() - layoutMargin;

    bool placeBelow = false;
    if (!canPlaceAbove && canPlaceBelow) {
        placeBelow = true;
    }
    else if (!canPlaceAbove && !canPlaceBelow) {
        const int visibleAbove = qMax(0, cursor.mouseY - yShift - layoutMargin);
        const int visibleBelow = qMax(0, canvas.height() - layoutMargin - (cursor.mouseY + yShift));
        placeBelow = visibleBelow > visibleAbove;
    }

    int layoutX = placeToRight ? rightStartX : cursor.mouseX - xShift - layoutWidth;
    int layoutY = placeBelow ? belowY : aboveY;
    layoutX = qBound(layoutMargin, layoutX, qMax(layoutMargin, canvas.width() - layoutMargin - layoutWidth));
    layoutY = qBound(layoutMargin, layoutY, qMax(layoutMargin, canvas.height() - layoutMargin - layoutHeight));

    int textBoxX = layoutX;
    int previewX = layoutX;
    if (hasPreview) {
        if (placeToRight) {
            textBoxX = layoutX;
            previewX = textBoxX + textBoxWidth + previewGap;
        }
        else {
            previewX = layoutX;
            textBoxX = previewX + previewSize + previewGap;
        }
    }

    const int textBoxY = layoutY + (layoutHeight - textBoxHeight) / 2;
    const QRect textBackgroundRect(textBoxX, textBoxY, textBoxWidth, textBoxHeight);
    textRect = textBackgroundRect.adjusted(textMargin, textMargin, -textMargin, -textMargin);

    p->setPen(Qt::NoPen);
    p->setBrush(QColor(45, 45, 45));
    p->drawRect(textBackgroundRect);

    p->setPen(QColor(255, 255, 255));
    p->drawText(textRect, Qt::AlignLeft | Qt::AlignTop, text);

    if (hasPreview) {
        const int previewY = layoutY + (layoutHeight - previewSize) / 2;
        QRect previewRect(previewX, previewY, previewSize, previewSize);
        QRect previewInnerRect = previewRect.adjusted(3, 3, -3, -3);
        QPoint sourceCenter(qBound(0, cursor.mouseX, canvas.width() - 1),
                            qBound(0, cursor.mouseY, canvas.height() - 1));

        p->setPen(Qt::NoPen);
        p->setBrush(QColor(30, 30, 30, 220));
        p->drawRect(previewRect);
        QPointF previewFocus(0.5, 0.5);
        parent->drawEchogramZoomPreview(p, previewInnerRect, sourceCenter, previewSourceSize, &previewFocus);

        p->setPen(QPen(QColor(94, 101, 132, 255), 2));
        p->setBrush(Qt::NoBrush);
        p->drawRect(previewRect.adjusted(1, 1, -1, -1));

        const qreal focusX = qBound<qreal>(0.0, previewFocus.x(), 1.0);
        const qreal focusY = qBound<qreal>(0.0, previewFocus.y(), 1.0);
        const int zoomCenterX = previewInnerRect.left()
            + qRound(focusX * static_cast<qreal>(qMax(0, previewInnerRect.width() - 1)));
        const int zoomCenterY = previewInnerRect.top()
            + qRound(focusY * static_cast<qreal>(qMax(0, previewInnerRect.height() - 1)));
        const int crossHalf = qMax(6 * scaleFactor_, previewInnerRect.width() / 10);
        const int leftArm = qMin(crossHalf, qMax(0, zoomCenterX - previewInnerRect.left()));
        const int rightArm = qMin(crossHalf, qMax(0, previewInnerRect.right() - zoomCenterX));
        const int topArm = qMin(crossHalf, qMax(0, zoomCenterY - previewInnerRect.top()));
        const int bottomArm = qMin(crossHalf, qMax(0, previewInnerRect.bottom() - zoomCenterY));
        p->setPen(QPen(QColor(255, 255, 255, 220), 2));
        p->drawLine(zoomCenterX - leftArm, zoomCenterY, zoomCenterX + rightArm, zoomCenterY);
        p->drawLine(zoomCenterX, zoomCenterY - topArm, zoomCenterX, zoomCenterY + bottomArm);
    }

    return true;
}

void Plot2DAim::setEpochEventState(bool state)
{
    beenEpochEvent_ = state;
}
