#include "stream_list.h"

#include <core.h>
extern Core core;

StreamList::StreamList(QObject* parent) : QObject(parent)
{
//    createStream(0);
//    _lastStream = getStream(0);
//    _lastStreamId = 0;
}

StreamList::~StreamList()
{
    if (!_updater) {
        return;
    }

    _updater->stop();
    delete _updater;
}

void StreamList::initTimer()
{
    if (_updater) {
        return;
    }

    _updater = new QTimer(this);
    connect(_updater, &QTimer::timeout, this, &StreamList::process);
    _updater->start(100);
}

void StreamList::debugAddGap(uint32_t start, uint32_t size) {
    Q_UNUSED(start);
    Q_UNUSED(size);
//    core.consoleInfo(QString("Find a gap %1 from %2").arg(size).arg(start));
}


void StreamList::debugSearchGap(uint32_t start, uint32_t size) {
    Q_UNUSED(start);
    Q_UNUSED(size);
//    core.consoleInfo(QString("Search a gap %1 from %2").arg(size).arg(start));
}


void StreamList::process() {
    if(_isInserting) return;

    QMapIterator<int, Stream> i(_streams);
     while (i.hasNext()) {
         i.next();
        // i.key(); i.value();
     }
}
