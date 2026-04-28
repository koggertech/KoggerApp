#include "plot2D_dvl_beam_velocity.h"
#include "plot2D.h"
#include <QPainter>
#include <QFontMetrics>
#include <QVector>


Plot2DDVLBeamVelocity::Plot2DDVLBeamVelocity()
{}

bool Plot2DDVLBeamVelocity::draw(Plot2D* parent, Dataset* dataset)
{
    auto& canvas = parent->canvas();
    auto& cursor = parent->cursor();

    _hasData = false;

    if (!isVisible() || !cursor.velocity.isValid()) {
        return false;
    }

    QVector<float> beam_velocity(canvas.width());
    beam_velocity.fill(NAN);
    QVector<float> beam_amp(canvas.width());
    beam_amp.fill(NAN);
    QVector<float> beam_mode(canvas.width());
    beam_mode.fill(NAN);
    QVector<float> beam_coh(canvas.width());
    beam_coh.fill(NAN);
    QVector<float> beam_dist(canvas.width());
    beam_dist.fill(NAN);

    for (int ibeam = 0; ibeam < 4; ibeam++) {
        const int base = ibeam * 3;
        const bool showV = (_beamFilter >> base)       & 1;
        const bool showA = (_beamFilter >> (base + 1)) & 1;
        const bool showM = (_beamFilter >> (base + 2)) & 1;

        if (!showV && !showA && !showM) {
            continue;
        }

        for (int i = 0; i < canvas.width(); i++) {
            int pool_index = cursor.getIndex(i);
            Epoch *data = dataset->fromIndex(pool_index);

            if (data != nullptr && data->isDopplerBeamAvail(ibeam)) {
                IDBinDVL::BeamSolution beam = data->dopplerBeam(ibeam);
                beam_velocity[i] = beam.velocity;
                beam_amp[i] = beam.amplitude * 0.1f;
                beam_mode[i] = (beam.mode * 6 + ibeam) * 2;
                beam_coh[i] = beam.coherence[0];
                beam_dist[i] = beam.distance;
                _hasData = true;
            }
        }

        if (showV)
            drawY(canvas, beam_velocity, cursor.velocity.from, cursor.velocity.to, _penBeam[ibeam]);
        if (showA)
            drawY(canvas, beam_dist, cursor.distance.from, cursor.distance.to, _penAmp[ibeam]);
        if (showM)
            drawY(canvas, beam_mode, canvas.height(), 0, _penMode[ibeam]);
    }

    return true;
}

void Plot2DDVLBeamVelocity::setBeamFilter(int filter)
{
    _beamFilter = filter;
}

int Plot2DDVLBeamVelocity::countLegendItems() const
{
    int n = 0;
    for (int i = 0; i < 4; i++) {
        const int base = i * 3;
        if ((_beamFilter >> base)       & 1) n++;
        if ((_beamFilter >> (base + 1)) & 1) n++;
        if ((_beamFilter >> (base + 2)) & 1) n++;
    }
    return n;
}

int Plot2DDVLBeamVelocity::drawLegend(Canvas& canvas, int x, int y)
{
    if (!isVisible() || !_hasData)
        return y;

    QPainter* p = canvas.painter();
    if (!p)
        return y;

    struct Item { QString label; QColor color; };
    QVector<Item> items;
    items.reserve(12);

    const QColor modeColors[4]  = { {255,   0, 150}, {  0, 155, 255}, {255, 175,   0}, { 75, 205,  55} };
    const QColor beamColors[4]  = { {171,   0, 100}, {  0, 104, 171}, {171, 117,   0}, { 50, 137,  37} };
    const QColor ampColors[4]   = { {115,   0,  67}, {  0,  70, 115}, {115,  78,   0}, { 34,  92,  25} };

    for (int i = 0; i < 4; i++) {
        const int base = i * 3;
        const QString n = QString::number(i + 1);
        if ((_beamFilter >> (base + 1)) & 1) items.append({n + " " + QObject::tr("Depth"),    ampColors[i]});
        if ((_beamFilter >> base)       & 1) items.append({n + " " + QObject::tr("Velocity"), beamColors[i]});
        if ((_beamFilter >> (base + 2)) & 1) items.append({n + " " + QObject::tr("Mode"),     modeColors[i]});
    }

    if (items.isEmpty())
        return y;

    const QString header   = QObject::tr("Doppler Beams");
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

int Plot2DDVLBeamVelocity::boxWidth(Canvas& canvas) const
{
    if (!isVisible() || !_hasData) return 0;
    QPainter* p = canvas.painter();
    if (!p) return 0;
    const QString header = QObject::tr("Doppler Beams");
    const QFont headerFont("Asap", 14, QFont::Normal);
    const QFont font      ("Asap", 14, QFont::Normal);
    const QFontMetrics hfm(headerFont);
    const QFontMetrics fm(font);
    const int lineW = 20, lineGap = 6, padX = 8;
    int maxItemW = 0;
    for (int i = 0; i < 4; i++) {
        const int base = i * 3;
        const QString n = QString::number(i + 1);
        if ((_beamFilter >> (base + 1)) & 1) maxItemW = qMax(maxItemW, lineW + lineGap + fm.horizontalAdvance(n + " " + QObject::tr("Depth")));
        if ((_beamFilter >> base)       & 1) maxItemW = qMax(maxItemW, lineW + lineGap + fm.horizontalAdvance(n + " " + QObject::tr("Velocity")));
        if ((_beamFilter >> (base + 2)) & 1) maxItemW = qMax(maxItemW, lineW + lineGap + fm.horizontalAdvance(n + " " + QObject::tr("Mode")));
    }
    if (maxItemW == 0) return 0;
    return padX + qMax(maxItemW, hfm.horizontalAdvance(header)) + padX;
}
