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

typedef enum {
    kManual = 0,
    kAuto,
    kAutoOnce
} ControlType;

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

    /*multi link*/
    bool getConnectionStatus() const;
    ControlType getControlType() const;
    /*Serial*/
    QString getPortName() const;
    int getBaudrate() const;
    bool getParity() const;
    /*UDP/TCP*/
    LinkType getLinkType() const;
    QString getAddress() const;
    int getSourcePort() const;
    int getDestinationPort() const;
    /*other*/
    bool isPinned() const;
    bool isHided() const;
    bool isNotAvailable() const;
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

    /*multi link*/
    ControlType controlType_;
    /*Serial*/
    QString portName_;
    int baudrate_;
    bool parity_;
    /*UDP/TCP*/
    LinkType linkType_;
    QString address_;
    int srcPort_;
    int dstPort_;
    /*others*/
    bool isPinned_;
    bool isHided_;
    bool isNotAvailable_;
    /**/

    void setType(LinkType type) { _type = type; }
    void setDev(QIODevice* dev);
    void deleteDev();

signals:
    void readyParse(Link* link);
    void changeState();
};


#endif // LINK_H
