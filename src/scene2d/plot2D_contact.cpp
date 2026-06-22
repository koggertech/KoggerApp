#include "plot2D_contact.h"
#include "plot2D.h"
#include "themes.h"

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
                p->save();
                if (vertical) {
                    p->translate(textRect.topLeft() - QPointF(2 * adjPix - qMax(1, qRound(2 * s)), 0));
                    p->rotate(90);
                }
                const QRectF boxRect = vertical ? QRectF(0, 0, textRect.width(), textRect.height()) : textRect;
                p->setPen(Qt::NoPen);
                p->setBrush(linesColor);
                p->drawRect(boxRect.adjusted(-adjPix, -adjPix, adjPix, adjPix));
                p->setBrush(QColor(45, 45, 45));
                p->drawRect(boxRect.adjusted(-3 * s, -3 * s, 3 * s, 3 * s));
                p->setPen(QColor(255, 255, 255));
                p->drawText(boxRect, Qt::AlignLeft | Qt::AlignTop, infoText);
                p->restore();

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
