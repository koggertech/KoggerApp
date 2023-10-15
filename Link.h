#ifndef LINK_H
#define LINK_H

#include <ProtoBinnary.h>

#include <QObject>
#include "QIODevice"
#include "QByteArray"
#include <QQueue>
#include "QMutex"
#include "QThread"

using namespace Parsers;

typedef enum {
    LinkNone,
    LinkUART,
    LinkIPUDP,
    LinkIPTCP,
} LinkType;

class Link : public QObject {
    Q_OBJECT
public:
    Link() {
    }

    void openAsUDP(const QString &address, const int port_in,  const int port_out);

    bool isOpen();
    void close();

    bool parse() {
        if(_frame.availContext() == 0) {
            if(_buffer.size() > 0) {
                _context = _buffer;
                _buffer.resize(0);
                _frame.setContext((uint8_t*)_context.data(), _context.size());
            }
        }

        _frame.process();
        return _frame.isComplete() || _frame.availContext();
    }


    FrameParser* frameParser() { return &_frame; }
    QIODevice* device() { return _dev; }

public slots:
    bool writeFrame(FrameParser* frame);
    bool write(QByteArray data);

private slots:
    void toContext(const QByteArray data);
    void readyRead();
    void aboutToClose();


private:
    QMutex _mutex;
    FrameParser _frame;

    QIODevice* _dev = nullptr;
    QByteArray _context;
    QByteArray _buffer;

    LinkType _type = LinkNone;
    void setType(LinkType type) { _type = type; }
    void setDev(QIODevice* dev);
    void deleteDev();

signals:
    void readyParse(Link* link);
    void changeState();
};


#endif // LINK_H
