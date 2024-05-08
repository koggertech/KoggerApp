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
    Link();
    Link(const Link& other); // TODO

    void openAsUDP(const QString &address, const int port_in,  const int port_out);
    void openAsSerial(const QString& name);

    bool isOpen();
    void close();

    bool parse() {
        if(_frame.availContext() == 0) {
            if(_buffer.size() > 0) {
                _context = QByteArray::fromRawData(_buffer.constData(), _buffer.size());
                _buffer.resize(0);
                _frame.setContext((uint8_t*)_context.data(), _context.size());
            }
        }

        _frame.process();
        return _frame.isComplete() || _frame.availContext();
    }


    FrameParser* frameParser() { return &_frame; }
    QIODevice* device() { return _dev; }

    Link& operator=(const Link& other) {
        this->_buffer = other._buffer;
        this->_context = other._context;
        this->_dev = other._dev;
        this->_frame = other._frame;
        this->_type = other._type;
        this->portName_ = other.portName_;

        return *this;
    }

    bool operator==(const Link& other) const { // TODO
        if (this->_type == other._type &&
            this->_dev == other._dev)
            return true;
        else
            return false;
    };    

    /*Serial*/
    QString getPortName() const;
    /**/

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

    /*Serial*/
    QString portName_;
    /**/

    void setType(LinkType type) { _type = type; }
    void setDev(QIODevice* dev);
    void deleteDev();

signals:
    void readyParse(Link* link);
    void changeState();
};


#endif // LINK_H
