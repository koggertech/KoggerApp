#include "plot2D_aim.h"
#include "plot2D.h"
#include "themes.h"
#include <cmath>


Plot2DAim::Plot2DAim()
    : beenEpochEvent_(false),
    lineWidth_(1),
    lineColor_(255, 255, 255, 255)
{
    scaleFactor_ = renderScale();
}

bool Plot2DAim::draw(Plot2D* parent, Dataset* dataset)
{
    scaleFactor_ = renderScale();
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
                float bottomDistance = parent->hasSyncDepth() ? parent->getSyncDepth() : chartPtr->bottomProcessing.distance;
                if (!std::isfinite(bottomDistance)) {
                    bottomDistance = 0.0f;
                }

                const float distanceRange = cursor.distance.range();
                int y = (cursor.channel2 != channelNone()) ? canvas.height() / 2 : 0;
                if (std::isfinite(distanceRange) && std::abs(distanceRange) > 1e-6f) {
                    const float depthNorm = (bottomDistance - cursor.distance.from) / distanceRange;
                    const float yFloat = (cursor.channel2 != channelNone())
                        ? static_cast<float>(canvas.height()) * 0.5f
                            - static_cast<float>(canvas.height()) * depthNorm
                        : static_cast<float>(canvas.height()) * depthNorm;
                    y = qRound(yFloat);
                }

                y = qBound(0, y, qMax(0, canvas.height() - 1));
                cursor.setMouse(x, y);
            }
        }
    }

    QPainter* p = canvas.painter();
    const bool vertical = !parent->isHorizontal();

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

    const int aimMask = parent->getAimFieldsMask();
    QString text;
    auto addLine = [&text](const QString& s) {
        if (!text.isEmpty())
            text += "\n";
        text += s;
    };

    if (aimMask & (1 << 0)) {
        addLine(QString(QObject::tr("%1 m")).arg(cursor_distance, 0, 'g', 4));
    }

    auto [channelId, subIndx, name] = parent->getSelectedChannelId();

    if ((aimMask & (1 << 1)) && channelId != channelNone()) {
        addLine(QObject::tr("Channel: ") + QString("%1").arg(name));
    }

    if (cursor.currentEpochIndx != -1) {
        if (aimMask & (1 << 2)) {
            addLine(QObject::tr("Epoch: ") + QString::number(cursor.currentEpochIndx));
        }

        if (auto* ep = dataset->fromIndex(cursor.currentEpochIndx); ep) {
            if (auto* echogram = ep->chart(channelId, subIndx); echogram) {
                if (!echogram->recordParameters_.isNull()) {
                    auto& recParams = echogram->recordParameters_;
                    if (aimMask & (1 << 3)) addLine(QObject::tr("Resolution, mm: ")      + QString::number(recParams.resol));
                    if (aimMask & (1 << 4)) addLine(QObject::tr("Frequency, kHz: ")      + QString::number(recParams.freq));
                    if (aimMask & (1 << 5)) addLine(QObject::tr("Pulse count: ")         + QString::number(recParams.pulse));
                    if (aimMask & (1 << 6)) addLine(QObject::tr("Booster: ")             + (recParams.boost ? QObject::tr("ON") : QObject::tr("OFF")));
                    if (aimMask & (1 << 7)) addLine(QObject::tr("Speed of sound, m/s: ") + QString::number(recParams.soundSpeed / 1000));
                }
            }
        }
    }

    QRect textRect = p->fontMetrics().boundingRect(QRect(0, 0, 9999, 9999), Qt::AlignLeft | Qt::AlignTop, text);

    const int xShift = 50 * scaleFactor_;
    const int yShift = 40 * scaleFactor_;
    const int textMargin = 5 * scaleFactor_;
    const QRect textBackgroundLocal = textRect.adjusted(-textMargin, -textMargin, textMargin, textMargin);
    const int textBoxWidth = text.isEmpty() ? 0 : textBackgroundLocal.width();
    const int textBoxHeight = text.isEmpty() ? 0 : textBackgroundLocal.height();

    const bool loupeEnabled = parent->getLoupeVisible();
    const int loupeSize = qBound(1, parent->getLoupeSize(), 3);
    const int loupeZoomPercent = qBound(0, parent->getLoupeZoom(), 300);
    const float loupeSizeMultiplier = (loupeSize == 1) ? 1.0f : ((loupeSize == 2) ? 1.5f : 2.25f);
    const float loupeZoomMultiplier = 1.0f + static_cast<float>(loupeZoomPercent) * 0.01f;
    const int previewFrameMargin = 10 * scaleFactor_;
    const int maxPreviewSize = qMin(canvas.width() - previewFrameMargin * 2, canvas.height() - previewFrameMargin * 2);
    const bool hasPreview = loupeEnabled && maxPreviewSize > 30 * scaleFactor_;
    const int previewSize = hasPreview ? qMin(static_cast<int>(180.0f * scaleFactor_ * loupeSizeMultiplier), maxPreviewSize) : 0;
    const int previewSourceBaseSize = hasPreview ? qMax(8, previewSize) : 0;
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
            previewX = layoutX;
            textBoxX = previewX + previewSize + previewGap;
        }
        else {
            textBoxX = layoutX;
            previewX = textBoxX + textBoxWidth + previewGap;
        }
    }

    int textBoxY = layoutY + (layoutHeight - textBoxHeight) / 2;
    int previewY = layoutY + (layoutHeight - previewSize) / 2;

    if (vertical) {
        // Mirror the horizontal HUD: a screen-row (loupe + params table) that follows
        // the cursor. Lay it out in screen space, then map to the rotated canvas.
        const int CW = canvas.width();
        const int screenW = canvas.height();
        const int screenH = canvas.width();
        const int sMargin = previewFrameMargin;
        const int rowW = textBoxWidth + (hasPreview ? previewGap + previewSize : 0);
        const int rowH = qMax(textBoxHeight, hasPreview ? previewSize : 0);

        const int curSX = qBound(0, cursor.mouseY, screenW - 1);
        const int curSY = qBound(0, CW - cursor.mouseX, screenH - 1);

        int rowSX = curSX + xShift;
        if (rowSX + rowW > screenW - sMargin && curSX - xShift - rowW >= sMargin) {
            rowSX = curSX - xShift - rowW;
        }
        rowSX = qBound(sMargin, rowSX, qMax(sMargin, screenW - sMargin - rowW));

        int rowSY = curSY - yShift - rowH;
        if (rowSY < sMargin) {
            rowSY = curSY + yShift;
        }
        rowSY = qBound(sMargin, rowSY, qMax(sMargin, screenH - sMargin - rowH));

        const int loupeSCx = rowSX + previewSize / 2;
        const int textSCx = rowSX + (hasPreview ? previewSize + previewGap : 0) + textBoxWidth / 2;
        const int rowSCy = rowSY + rowH / 2;
        previewX = (CW - rowSCy) - previewSize / 2;
        previewY = loupeSCx - previewSize / 2;
        textBoxX = (CW - rowSCy) - textBoxWidth / 2;
        textBoxY = textSCx - textBoxHeight / 2;
    }

    const QRect textBackgroundRect(textBoxX, textBoxY, textBoxWidth, textBoxHeight);
    textRect = textBackgroundRect.adjusted(textMargin, textMargin, -textMargin, -textMargin);

    if (!text.isEmpty()) {
        p->save();
        if (vertical) {
            const QPointF c = textBackgroundRect.center();
            p->translate(c);
            p->rotate(90);
            p->translate(-c);
        }
        p->setPen(Qt::NoPen);
        p->setBrush(QColor(45, 45, 45));
        p->drawRect(textBackgroundRect);

        p->setPen(QColor(255, 255, 255));
        p->drawText(textRect, Qt::AlignLeft | Qt::AlignTop, text);
        p->restore();
    }

    if (hasPreview) {
        QRect previewRect(previewX, previewY, previewSize, previewSize);
        QRect previewInnerRect = previewRect.adjusted(3, 3, -3, -3);
        QPoint sourceCenter(qBound(0, cursor.mouseX, canvas.width() - 1),
                            qBound(0, cursor.mouseY, canvas.height() - 1));

        p->save();
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
        const int crossHalf = qMax(qRound(6 * scaleFactor_), previewInnerRect.width() / 10);
        const int leftArm = qMin(crossHalf, qMax(0, zoomCenterX - previewInnerRect.left()));
        const int rightArm = qMin(crossHalf, qMax(0, previewInnerRect.right() - zoomCenterX));
        const int topArm = qMin(crossHalf, qMax(0, zoomCenterY - previewInnerRect.top()));
        const int bottomArm = qMin(crossHalf, qMax(0, previewInnerRect.bottom() - zoomCenterY));
        p->setPen(QPen(QColor(255, 255, 255, 220), 2));
        p->drawLine(zoomCenterX - leftArm, zoomCenterY, zoomCenterX + rightArm, zoomCenterY);
        p->drawLine(zoomCenterX, zoomCenterY - topArm, zoomCenterX, zoomCenterY + bottomArm);
        p->restore();
    }

    return true;
}

void Plot2DAim::setEpochEventState(bool state)
{
    beenEpochEvent_ = state;
}
