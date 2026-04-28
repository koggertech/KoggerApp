#include "plot2D_dvl_solution.h"
#include "plot2D.h"
#include <QPainter>
#include <QFontMetrics>
#include <QVector>


Plot2DDVLSolution::Plot2DDVLSolution()
{}

bool Plot2DDVLSolution::draw(Plot2D *parent, Dataset *dataset)
{
    auto& canvas = parent->canvas();
    auto& cursor = parent->cursor();

    _hasData = false;

    if (!isVisible() || !cursor.velocity.isValid()) {
        return false;
    }

    QVector<float> velocityX(canvas.width());
    velocityX.fill(NAN);

    QVector<float> velocityY(canvas.width());
    velocityY.fill(NAN);

    QVector<float> velocityZ(canvas.width());
    velocityZ.fill(NAN);

    QVector<float> velocityA(canvas.width());
    velocityA.fill(NAN);

    QVector<float> distance(canvas.width());
    distance.fill(NAN);

    for (int i = 0; i < canvas.width(); i++) {
        int pool_index = cursor.getIndex(i);
        Epoch *data = dataset->fromIndex(pool_index);

        if (data != nullptr && data->isDVLSolutionAvail()) {
            IDBinDVL::DVLSolution sol = data->dvlSolution();
            velocityX[i] = sol.velocity.x;
            velocityY[i] = sol.velocity.y;
            velocityZ[i] = sol.velocity.z;
            velocityA[i] = sqrt(sol.velocity.x * sol.velocity.x +
                                sol.velocity.y * sol.velocity.y +
                                sol.velocity.z * sol.velocity.z);
            distance[i] = sol.distance.z;
            _hasData = true;
        }
    }

    if (_lineFilter & LineX)
        drawY(canvas, velocityX, cursor.velocity.from, cursor.velocity.to, _penVelocity[0]);
    if (_lineFilter & LineY)
        drawY(canvas, velocityY, cursor.velocity.from, cursor.velocity.to, _penVelocity[1]);
    if (_lineFilter & LineZ)
        drawY(canvas, velocityZ, cursor.velocity.from, cursor.velocity.to, _penVelocity[2]);
    if (_lineFilter & LineAbs)
        drawY(canvas, velocityA, cursor.velocity.from, cursor.velocity.to, _penVelocity[3]);
    if (_lineFilter & LineDst)
        drawY(canvas, distance, cursor.distance.from, cursor.distance.to, _penDist);

    return true;
}

int Plot2DDVLSolution::countLegendItems() const
{
    int n = 0;
    if (_lineFilter & LineX)   n++;
    if (_lineFilter & LineY)   n++;
    if (_lineFilter & LineZ)   n++;
    if (_lineFilter & LineAbs) n++;
    if (_lineFilter & LineDst) n++;
    return n;
}

int Plot2DDVLSolution::drawLegend(Canvas& canvas, int x, int y)
{
    if (!isVisible() || !_hasData)
        return y;

    QPainter* p = canvas.painter();
    if (!p)
        return y;

    struct Item { LineFlag flag; QString label; QColor color; };
    const Item defs[] = {
        { LineX,   "X",    QColor(255,   0,   0) },
        { LineY,   "Y",    QColor(180,   0, 255) },
        { LineZ,   "Z",    QColor(  0, 210, 210) },
        { LineAbs, QObject::tr("Abs. Velocity"), QColor(255, 220,   0) },
        { LineDst, QObject::tr("Depth"),        QColor(160, 160, 160) },
    };

    struct VisItem { QString label; QColor color; };
    QVector<VisItem> items;
    items.reserve(5);
    for (const auto& d : defs) {
        if (_lineFilter & d.flag)
            items.append({d.label, d.color});
    }

    if (items.isEmpty())
        return y;

    const QString header   = QObject::tr("Doppler Instr.");
    const QFont headerFont("Asap", 14, QFont::Normal);
    const QFont font      ("Asap", 14, QFont::Normal);
    const QFontMetrics hfm(headerFont);
    const QFontMetrics fm(font);
    const int lineW = 20, lineGap = 6;
    const int padX = 8, padY = 6;
    const int rowH    = fm.height() + 4;
    const int headerH = hfm.height() + 6;

    int maxItemW = 0;
    for (const auto& item : items)
        maxItemW = qMax(maxItemW, lineW + lineGap + fm.horizontalAdvance(item.label));

    const int boxW = padX + qMax(maxItemW, hfm.horizontalAdvance(header)) + padX;
    const int boxH = padY + headerH + items.size() * rowH + padY;

    const auto prevMode = p->compositionMode();
    p->setCompositionMode(QPainter::CompositionMode_SourceOver);
    p->setPen(Qt::NoPen);
    p->setBrush(QColor(0, 0, 0, 140));
    p->drawRoundedRect(QRect(x, y, boxW, boxH), 6, 6);

    // header
    p->setFont(headerFont);
    p->setPen(QPen(QColor(210, 210, 210)));
    p->drawText(x + padX, y + padY + hfm.ascent(), header);

    // items
    p->setFont(font);
    int iy = y + padY + headerH + fm.ascent();
    for (const auto& item : items) {
        const int lineY = iy - fm.ascent() + fm.height() / 2;
        p->setPen(QPen(item.color, 2));
        p->drawLine(x + padX, lineY, x + padX + lineW, lineY);
        p->setPen(QPen(item.color));
        p->drawText(x + padX + lineW + lineGap, iy, item.label);
        iy += rowH;
    }

    p->setCompositionMode(prevMode);
    return y + boxH + 4;
}

int Plot2DDVLSolution::boxWidth(Canvas& canvas) const
{
    if (!isVisible() || !_hasData) return 0;
    QPainter* p = canvas.painter();
    if (!p) return 0;
    const QString header = QObject::tr("Doppler Instr.");
    const QFont headerFont("Asap", 14, QFont::Normal);
    const QFont font      ("Asap", 14, QFont::Normal);
    const QFontMetrics hfm(headerFont);
    const QFontMetrics fm(font);
    const int lineW = 20, lineGap = 6, padX = 8;
    const QString labels[] = { "X", "Y", "Z", QObject::tr("Abs. Velocity"), QObject::tr("Depth") };
    const int flags[] = { LineX, LineY, LineZ, LineAbs, LineDst };
    int maxItemW = 0;
    for (int i = 0; i < 5; i++) {
        if (_lineFilter & flags[i])
            maxItemW = qMax(maxItemW, lineW + lineGap + fm.horizontalAdvance(labels[i]));
    }
    if (maxItemW == 0) return 0;
    return padX + qMax(maxItemW, hfm.horizontalAdvance(header)) + padX;
}
