#include "plot2D_contact.h"
#include "plot2D.h"
#include "themes.h"

#include <QPainterPath>

namespace {

QPainterPath squarePointerCornerBox(const QRectF& r, qreal radius)
{
    const qreal rad = qBound(0.0, radius, qMin(r.width(), r.height()) * 0.5);
    const qreal d = rad * 2.0;
    QPainterPath path;
    path.moveTo(r.topLeft());
    path.lineTo(r.right() - rad, r.top());
    path.arcTo(QRectF(r.right() - d, r.top(), d, d), 90.0, -90.0);
    path.lineTo(r.right(), r.bottom() - rad);
    path.arcTo(QRectF(r.right() - d, r.bottom() - d, d, d), 0.0, -90.0);
    path.lineTo(r.left() + rad, r.bottom());
    path.arcTo(QRectF(r.left(), r.bottom() - d, d, d), 270.0, -90.0);
    path.closeSubpath();
    return path;
}

} // namespace

bool Plot2DContact::draw(Plot2D *parent, Dataset *dataset)
{    
    auto& canvas = parent->canvas();
    auto& cursor = parent->cursor();

    const double s = renderScale();

    QPen pen;
    pen.setWidth(qMax(1, qRound(lineWidth_ * s)));
    pen.setColor(lineColor_);

    QPainter* p = canvas.painter();
    p->setPen(pen);
    QFont font = QFont("Asap", 14, QFont::Normal);
    font.setPixelSize(qRound(18 * s));
    p->setFont(font);
    p->setCompositionMode(QPainter::CompositionMode_SourceOver);
    qreal adjPix = 5 * s;
    qreal shiftXY = 20 * s;

    setVisibleContact(false);

    auto activeContactIndx = dataset->getActiveContactIndx();
    for (auto& indx : cursor.indexes) {
        auto* epoch = dataset->fromIndex(indx);

        if (!epoch) {
            continue;
        }

        bool isActiveContact = activeContactIndx == indx;
        if (epoch->contact_.isValid()) {
            const bool vertical = !isHorizontal_;
            float xPos = cursor.numZeroEpoch + indx - cursor.indexes[0];
            const float canvasHeight = canvas.height();
            float valueRange = cursor.distance.to - cursor.distance.from;
            float valueScale = canvasHeight / valueRange;
            float yPos = (epoch->contact_.echogramDistance - cursor.distance.from) * valueScale;
            bool intersects = false;

            auto& epRect = epoch->contact_.rectEcho;
            if (!epRect.isEmpty()) {
                QRectF locRect = vertical
                    ? QRectF(xPos + shiftXY - 2 * adjPix + qMax(1, qRound(2 * s)) - epRect.height(), yPos + shiftXY, epRect.height(), epRect.width())
                    : epRect.translated(QPointF(xPos + shiftXY, yPos + shiftXY) - epRect.topLeft());
                locRect = locRect.adjusted(-adjPix, -adjPix, adjPix, adjPix);

                if (locRect.contains(QPointF(mouseX_, mouseY_))) {
                    indx_ = indx;
                    isActive_ = isActiveContact;

                    if (isHorizontal_) {
                        position_ = QPoint(xPos + shiftXY * 0.75f, yPos + shiftXY * 0.75f);

                    }
                    else {
                        float fixedX = xPos + shiftXY;
                        float fixedY = yPos + shiftXY;
                        const float currDepth = parent->getDepthByMousePos(fixedX, fixedY, true);
                        const int currEpochIndx = parent->getEpochIndxByMousePos(fixedX, fixedY, true);
                        position_ = parent->getMousePosByDepthAndEpochIndx(currDepth, currEpochIndx, false);
                    }

                    info_ = epoch->contact_.info;
                    lat_ = epoch->contact_.lat;
                    lon_ = epoch->contact_.lon;
                    depth_ = epoch->contact_.depth;// echogramDistance;
                    setVisibleContact(true);
                    intersects = true;
                }
            }

            QString infoText = epoch->contact_.info;
            QRectF textRect = p->fontMetrics().boundingRect(infoText);
            textRect.moveTopLeft(QPointF(xPos + shiftXY, yPos + shiftXY));

            // write rect
            if (epRect.height() != textRect.height() ||
                epRect.width() != textRect.width()) {
                epRect = textRect;
            }

            if (intersects) {
                QPointF topLeft = textRect.adjusted(-adjPix + 1, -adjPix + 1, adjPix, adjPix).topLeft();
                p->setPen(QPen(QColor(0,190,0), qMax(1, qRound(2 * s))));
                p->drawLine(topLeft, topLeft + QPointF(-30 * s, 0));
                p->drawLine(topLeft, topLeft + QPointF(0, -30 * s));
            }
            else {
                QColor linesColor = isActiveContact ? QColor(0, 0, 190) : QColor(190, 0, 0);

                // Counter-rotate +90 in vertical so the label stays upright; lower the box
                // by 2*adjPix so its corner lands on the pointer corner (left unmoved).
                const bool prevAntialias = p->testRenderHint(QPainter::Antialiasing);
                p->save();
                if (vertical) {
                    p->translate(textRect.topLeft() - QPointF(2 * adjPix - qMax(1, qRound(2 * s)), 0));
                    p->rotate(90);
                }
                const QRectF boxRect = vertical ? QRectF(0, 0, textRect.width(), textRect.height()) : textRect;
                const qreal outerRadius = qMax(2.0, 5.0 * s);
                const qreal innerRadius = qMax(1.0, outerRadius - (adjPix - 3.0 * s));
                p->setRenderHint(QPainter::Antialiasing, true);
                p->setPen(Qt::NoPen);
                p->setBrush(linesColor);
                p->drawPath(squarePointerCornerBox(boxRect.adjusted(-adjPix, -adjPix, adjPix, adjPix),
                                                   outerRadius));
                p->setBrush(QColor(45, 45, 45));
                p->drawPath(squarePointerCornerBox(boxRect.adjusted(-3 * s, -3 * s, 3 * s, 3 * s),
                                                   innerRadius));
                p->setPen(QColor(255, 255, 255));
                p->drawText(boxRect, Qt::AlignLeft | Qt::AlignTop, infoText);
                p->restore();
                p->setRenderHint(QPainter::Antialiasing, prevAntialias);

                QPointF topLeft = textRect.adjusted(-adjPix + 1, -adjPix + 1, adjPix, adjPix).topLeft();
                p->setPen(QPen(linesColor, qMax(1, qRound(2 * s))));
                p->drawLine(topLeft, topLeft + QPointF(-30 * s, 0));
                p->drawLine(topLeft, topLeft + QPointF(0, -30 * s));
            }
        }
    }

    return true;
}

void Plot2DContact::setMousePos(int x, int y)
{
    mouseX_ = x;
    mouseY_ = y;
}

QString Plot2DContact::getInfo()
{
    return info_;
}

void Plot2DContact::setInfo(const QString &info)
{
    //qDebug() << "Plot2DContact::setInfo";

    info_ = info;
}

bool Plot2DContact::getVisible()
{
    return visible_;
}

void Plot2DContact::setVisible(bool visible)
{
    visible_ = visible;
}

QPoint Plot2DContact::getPosition()
{
    return position_;
}

int Plot2DContact::getIndx()
{
    return indx_;
}

double Plot2DContact::getLat()
{
    return lat_;
}

double Plot2DContact::getLon()
{
    return lon_;
}

double Plot2DContact::getDepth()
{
    return depth_;
}
bool Plot2DContact::isChanged() {
    if (visibleChanged_) {
        visibleChanged_ = false;
        return true;
    }
    return false;
}
void Plot2DContact::setVisibleContact(bool val) {
    if (visible_ != val) {
        visibleChanged_ = true;
    }
    visible_ = val;
}
