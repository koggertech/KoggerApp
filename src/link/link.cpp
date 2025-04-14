#include "link.h"


Link::Link() :
    ioDevice_(nullptr),
    uuid_(QUuid::createUuid()),
    controlType_(ControlType::kManual),
    linkType_(LinkType::kLinkNone),
    portName_(""),
    baudrate_(0),
    parity_(false),
    address_(""),
    sourcePort_(0),
    destinationPort_(0),
    isPinned_(false),
    isHided_(false),
    isNotAvailable_(false),
    isProxy_(false),
    isForcedStopped_(false),
    attribute_(0),
    autoSpeedSelection_(false),
    timeoutCnt_(linkNumTimeoutsSmall),
    lastTotalCnt_(0),
    isReceivesData_(false)
{
    frame_.resetComplete();

    checkTimer_ = std::make_unique<QTimer>(this);
    checkTimer_->setInterval(linkCheckingTimeInterval);

    QObject::connect(checkTimer_.get(), &QTimer::timeout, this, &Link::onCheckedTimerEnd, Qt::AutoConnection);
    QTimer::singleShot(500, this, [&]() -> void { checkTimer_->start(); });
}

void Link::createAsSerial(const QString &portName, int baudrate, bool parity)
{
    linkType_ = LinkType::kLinkSerial;
    portName_ = portName;
    parity_ = parity;
    baudrate_ = baudrate;
}

void Link::openAsSerial()
{
    if (autoSpeedSelection_) {
        baudrateSearchList_ = baudrateSearchList;
    }

    QSerialPort *serialPort = new QSerialPort(this);

    serialPort->setPortName(portName_);
    parity_ ? serialPort->setParity(QSerialPort::EvenParity) : serialPort->setParity(QSerialPort::NoParity);
    serialPort->setBaudRate(baudrate_);

    serialPort->setReadBufferSize(8 * 1024 * 1024);
    serialPort->open(QIODevice::ReadWrite);

    if (serialPort->isOpen()) {
        setDev(serialPort);
        emit connectionStatusChanged(uuid_);
        emit opened(uuid_, this);
    }
    else {
        delete serialPort;
        emit connectionStatusChanged(uuid_);
    }
}

void Link::createAsUdp(const QString &address, int sourcePort, int destinationPort)
{
    linkType_ = LinkType::kLinkIPUDP;
    address_ = address;
    sourcePort_ = sourcePort;
    destinationPort_ = destinationPort;
}

void Link::updateUdpParameters(const QString& address, int sourcePort, int destinationPort)
{
    address_ = address;
    sourcePort_ = sourcePort;
    destinationPort_ = destinationPort;
}

void Link::openAsUdp()
{
    QUdpSocket *socketUdp = new QUdpSocket(this);

    bool isBinded = socketUdp->bind(QHostAddress::AnyIPv4, sourcePort_); // , QAbstractSocket::ReuseAddressHint | QAbstractSocket::ShareAddress

    if (!isBinded) {
        delete socketUdp;
        return;
    }

    hostAddress_.setAddress(address_);

    if (isBinded) {
        socketUdp->open(QIODevice::ReadWrite);
        setDev(socketUdp);
        emit connectionStatusChanged(uuid_);
        emit opened(uuid_, this);
    }
    else {
        delete socketUdp;
        emit connectionStatusChanged(uuid_);
    }
}

void Link::createAsTcp(const QString &address, int sourcePort, int destinationPort)
{
    linkType_ = LinkType::kLinkIPTCP;
    address_ = address;
    sourcePort_ = sourcePort;
    destinationPort_ = destinationPort;
}

void Link::updateTcpParameters(const QString& address, int sourcePort, int destinationPort)
{
    address_ = address;
    sourcePort_ = sourcePort;
    destinationPort_ = destinationPort;
}

void Link::openAsTcp()
{
    // TODO
}

bool Link::isOpen() const
{
    bool retVal{ false };

    if (!ioDevice_)
        return retVal;

    switch (linkType_) {
    case LinkType::kLinkNone: {
        retVal = false;
        break;
    }
    case LinkType::kLinkSerial: {
        retVal = ioDevice_->isOpen();
        break;
    }
    case LinkType::kLinkIPUDP: {
        if (auto ptr = static_cast<QUdpSocket*>(ioDevice_); ptr)
            retVal = ptr->state() != QAbstractSocket::UnconnectedState;
        break;
    }
    case LinkType::kLinkIPTCP: {
        if (auto ptr = static_cast<QTcpSocket*>(ioDevice_); ptr)
            retVal = ptr->state() != QAbstractSocket::UnconnectedState;
        break;
    }
    default:
        break;
    }

    return retVal;
}

void Link::close()
{
    deleteDev();
}

bool Link::parse()
{
    if (frame_.availContext() == 0) {
        if (buffer_.size() > 0) {
            context_ = QByteArray::fromRawData(buffer_.constData(), buffer_.size());
            buffer_.resize(0);
            frame_.setContext((uint8_t*)context_.data(), context_.size());
        }
    }

    frame_.process();
    return frame_.isComplete() || frame_.availContext();
}

FrameParser *Link::frameParser()
{
    return &frame_;
}

QIODevice *Link::device()
{
    return ioDevice_;
}

void Link::setUuid(QUuid uuid)
{
    uuid_ = uuid;
}

void Link::setConnectionStatus(bool connectionStatus)
{
    if (!connectionStatus)
        close();
    else {
        switch (linkType_) {
        case LinkType::kLinkNone: { break; }
        case LinkType::kLinkSerial: { openAsSerial(); break; }
        case LinkType::kLinkIPUDP: { openAsUdp(); break; }
        case LinkType::kLinkIPTCP: { openAsTcp(); break; }
        default: { break; }
        }
    }
}

void Link::setControlType(ControlType controlType)
{
    controlType_ = controlType;
}

void Link::setPortName(const QString &portName)
{
    portName_ = portName;
}

void Link::setBaudrate(int baudrate, bool fromAutoSelector)
{
    baudrate_ = baudrate;

    if (linkType_ == LinkType::kLinkSerial) {
        if (auto currDev = static_cast<QSerialPort*>(ioDevice_); currDev) {
            currDev->setBaudRate(baudrate_);
        }

        if (!fromAutoSelector) {
            baudrateSearchList_.clear();
        }
    }
}

void Link::setParity(bool parity)
{
    parity_ = parity;
}

void Link::setLinkType(LinkType linkType)
{
    linkType_ = linkType;
}

void Link::setAddress(const QString &address)
{
    address_ = address;
    //hostAddress_.setAddress(address_);
    // TODO: rebind?
}

void Link::setSourcePort(int sourcePort)
{
    sourcePort_ = sourcePort;
    // TODO: rebind?
}

void Link::setDestinationPort(int destinationPort)
{
    destinationPort_ = destinationPort;
    // TODO: rebind?
}

void Link::setIsPinned(bool state)
{
    isPinned_ = state;
}

void Link::setIsHided(bool isHided)
{
    isHided_ = isHided;
}

void Link::setIsNotAvailable(bool isNotAvailable)
{
    isNotAvailable_ = isNotAvailable;
}

void Link::setIsProxy(bool isProxy)
{
    isProxy_ = isProxy;
}

void Link::setIsForceStopped(bool isForcedStopped)
{
    isForcedStopped_ = isForcedStopped;
}

void Link::setAutoSpeedSelection(bool autoSpeedSelection)
{
    autoSpeedSelection_ = autoSpeedSelection;

    if (autoSpeedSelection_) {
        baudrateSearchList_ = baudrateSearchList;
    }
}

QUuid Link::getUuid() const
{
    return uuid_;
}

bool Link::getConnectionStatus() const
{
    if (ioDevice_ && ioDevice_->isOpen()) {
        return true;
    }
    return false;
}

bool Link::getIsRecievesData() const
{
    return isReceivesData_;
}

ControlType Link::getControlType() const
{
    return controlType_;
}

QString Link::getPortName() const
{
    return portName_;
}

int Link::getBaudrate() const
{
    return baudrate_;
}

bool Link::getParity() const
{
    return parity_;
}

LinkType Link::getLinkType() const
{
    return linkType_;
}

QString Link::getAddress() const
{
    return address_;
}

int Link::getSourcePort() const
{
    return sourcePort_;
}

int Link::getDestinationPort() const
{
    return destinationPort_;
}

bool Link::getIsPinned() const
{
    return isPinned_;
}

bool Link::getIsHided() const
{
    return isHided_;
}

bool Link::getIsNotAvailable() const
{
    return isNotAvailable_;
}

bool Link::getIsProxy() const
{
    return isProxy_;
}

bool Link::getIsForceStopped() const
{
    return isForcedStopped_;
}

bool Link::getAutoSpeedSelection() const
{
    return autoSpeedSelection_;
}

// #ifdef MOTOR
// void Link::setIsMotorDevice(bool isMotorDevice)
// {
//     isMotorDevice_ = isMotorDevice;
// }

// bool Link::getIsMotorDevice() const
// {
//     return isMotorDevice_;
// }
// #endif

bool Link::writeFrame(FrameParser frame)
{
    return frame.isComplete() && write(QByteArray((const char*)frame.frame(), frame.frameLen()));
}

bool Link::write(QByteArray data)
{
    QIODevice *dev = device();
    if(dev != nullptr && isOpen()) {
        if(linkType_ == LinkType::kLinkIPUDP) {
            (static_cast<QUdpSocket*>(ioDevice_))->writeDatagram(data, hostAddress_, destinationPort_);
        } else {
            ioDevice_->write(data);
        }
        return true;
    }
    return false;
}

void Link::onCheckedTimerEnd()
{
    if (!getConnectionStatus()) {
        return;
    }

    auto currTotalCnt       = frame_.getCompleteTotal();
    bool lastIsReceivesData = isReceivesData_;
    bool newDataReceived    = (currTotalCnt != lastTotalCnt_);

    // timeouts
    if (newDataReceived) {
        isReceivesData_ = true;
        timeoutCnt_ = linkNumTimeoutsBig;
    }
    else {
        if (timeoutCnt_ > 0) {
            --timeoutCnt_;
        }

        if (!timeoutCnt_ && isReceivesData_) {
            isReceivesData_ = false;

            if (autoSpeedSelection_) {
                baudrateSearchList_ = baudrateSearchList;
                timeoutCnt_ = linkNumTimeoutsSmall;
            }
        }
    }

    // gui
    if (lastIsReceivesData != isReceivesData_) {
        emit isReceivesDataChanged(uuid_);
    }

    // autosearch
    if (autoSpeedSelection_ &&
        !isReceivesData_ &&
        !timeoutCnt_ &&
        !baudrateSearchList_.empty()) {
        auto currBaudrate = baudrateSearchList_.takeFirst();
        qDebug() << " trying check" << currBaudrate;
        setBaudrate(currBaudrate, true);
        emit baudrateChanged(uuid_);
    }

    lastTotalCnt_ = currTotalCnt;
    checkTimer_->start();
}

void Link::setDev(QIODevice *dev)
{
    deleteDev();
    if (dev != nullptr) {
        ioDevice_ = dev;

        connect(dev, &QAbstractSocket::readyRead, this, &Link::readyRead);
        connect(dev, &QAbstractSocket::aboutToClose, this, &Link::aboutToClose);

        if (auto* socket = qobject_cast<QSerialPort*>(ioDevice_); socket) {
#if defined(Q_OS_ANDROID)
//    connect(m_serial, &QSerialPort::error, this, &Connection::handleSerialError);
#else
            connect(socket, &QSerialPort::errorOccurred, this, &Link::handleSerialError);
#endif
        }
    }
}

void Link::deleteDev()
{
    if (ioDevice_ != nullptr) {
        if (ioDevice_->isOpen()) {
            ioDevice_->close();
            emit connectionStatusChanged(uuid_);  // ???
            //emit closed(uuid_, this)
        }

        ioDevice_->disconnect(this);
        this->disconnect(ioDevice_);
        //setLinkType(LinkNone);
        delete ioDevice_;
    }

    ioDevice_ = nullptr;
}

void Link::toParser(const QByteArray data)
{
    if (data.size() <= 0) {
        return;
    }

    frame_.setContext((uint8_t*)data.data(), data.size());

    while (frame_.availContext() > 0) {
        frame_.process();
        if (frame_.isComplete()) {
            emit frameReady(uuid_, this, frame_);
        }
    }
}

void Link::readyRead()
{

#ifdef MOTOR
    QIODevice* dev = device();
    if (dev != nullptr) {

        if (attribute_ == LinkAttributeNone) {
            if (linkType_ == LinkType::LinkIPUDP) {
                QUdpSocket* socsUpd = (QUdpSocket*)dev;
                while (socsUpd->hasPendingDatagrams()) {
                    QByteArray datagram;
                    datagram.resize(socsUpd->pendingDatagramSize());
                    QHostAddress sender;
                    quint16 senderPort;
                    qint64 slen = socsUpd->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
                    if (slen == -1) {
                        break;
                    }
                    toParser(datagram);
                }
            }
            else {
                toParser(dev->readAll());
            }
        }
        else {
            if (linkType_ == LinkType::LinkIPUDP) {
                QUdpSocket* socsUpd = (QUdpSocket*)dev;
                while (socsUpd->hasPendingDatagrams()) {
                    QByteArray datagram;
                    datagram.resize(socsUpd->pendingDatagramSize());
                    QHostAddress sender;
                    quint16 senderPort;
                    qint64 slen = socsUpd->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
                    if (slen == -1) {
                        break;
                    }
                    emit dataReady(datagram);
                }
            }
            else {
                emit dataReady(dev->readAll());
            }
        }
    }
#else
    QIODevice* dev = device();
    if (dev != nullptr) {
        if (linkType_ == LinkType::kLinkIPUDP) {
            QUdpSocket* socsUpd = (QUdpSocket*)dev;
            while (socsUpd->hasPendingDatagrams()) {
                QByteArray datagram;
                datagram.resize(socsUpd->pendingDatagramSize());
                QHostAddress sender;
                quint16 senderPort;
                qint64 slen = socsUpd->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
                if (slen == -1) {
                    break;
                }
                toParser(datagram);
            }
        }
        else {
            toParser(dev->readAll());
        }
    }
#endif

}

void Link::aboutToClose()
{
    QIODevice *dev = device();
    if (dev != nullptr) {
        //emit changeState(); //
        emit connectionStatusChanged(uuid_);
        emit closed(uuid_, this);
    }
}

void Link::handleSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        close();
    }
}
