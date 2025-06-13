#include "link.h"


Link::Link()
    : ioDevice_(nullptr),
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
      attribute_(LinkAttribute::kLinkAttributeNone),
      autoSpeedSelection_(false),
      timeoutCnt_(linkNumTimeoutsSmall),
      lastTotalCnt_(0),
      isReceivesData_(false),
      lastSearchIndx_(0),
      onUpgradingFirmware_(false),
      localGhostIgnoreCount_(0),
      requestCnt_(requestAllCntBig),
      autoConnOnce_(false)
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
    uuid_ =  QUuid::createUuidV3(QUuid{}, portName_);
    parity_ = parity;
    baudrate_ = baudrate;
    baudrateSearchList_ = baudrateSearchList; // by default
    resetLastSearchIndx();
}

void Link::openAsSerial()
{
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
    baudrateSearchList_ = baudrateSearchList;
    resetLastSearchIndx();
}

void Link::createAsUdp(const QString &address, int sourcePort, int destinationPort)
{
    linkType_ = LinkType::kLinkIPUDP;
    address_ = address;
    sourcePort_ = sourcePort;
    destinationPort_ = destinationPort;
    uuid_ = QUuid::createUuid();
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
    uuid_ = QUuid::createUuid();
}

void Link::updateTcpParameters(const QString& address, int sourcePort, int destinationPort)
{
    address_ = address;
    sourcePort_ = sourcePort;
    destinationPort_ = destinationPort;
}

void Link::openAsTcp()
{
    QTcpSocket *socketTcp = new QTcpSocket(this);
    hostAddress_.setAddress(address_);

    QPointer<QTcpSocket> safeSock = socketTcp;

    connect(socketTcp, &QTcpSocket::connected, this, [=]() {
        if (!safeSock) {
            return;
        }

        setDev(safeSock);
        emit connectionStatusChanged(uuid_);
        emit opened(uuid_, this);
    });

    connect(socketTcp, &QTcpSocket::disconnected, this, [=]() {
        if (safeSock) {
            safeSock->deleteLater();
        }

        emit connectionStatusChanged(uuid_);
        close();
    });

    connect(socketTcp, &QTcpSocket::errorOccurred, this, [=](QAbstractSocket::SocketError err) {
        Q_UNUSED(err);
        if (safeSock && safeSock->state() != QAbstractSocket::ConnectedState) {
            safeSock->abort();
        }
        emit connectionStatusChanged(uuid_);
    });

    socketTcp->connectToHost(hostAddress_, destinationPort_);
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
        if (auto ptr = qobject_cast<QUdpSocket*>(ioDevice_); ptr)
            retVal = ptr->state() != QAbstractSocket::UnconnectedState;
        break;
    }
    case LinkType::kLinkIPTCP: {
        if (auto ptr = qobject_cast<QTcpSocket*>(ioDevice_); ptr)
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

void Link::setBaudrate(int baudrate)
{
    int lastBaudRate = baudrate_;

    baudrate_ = baudrate;

    bool installed = false;
    if (linkType_ == LinkType::kLinkSerial) {
        if (auto currDev = qobject_cast<QSerialPort*>(ioDevice_); currDev) {
            installed = currDev->setBaudRate(baudrate_);
        }
    }

    if (getConnectionStatus()) {
        if (!installed) {
            baudrate_ = lastBaudRate;
            emit baudrateChanged(uuid_);
        }
    }
}

void Link::setRequestToSend(bool rts) {
    bool installed = false;
    if (linkType_ == LinkType::kLinkSerial) {
        if (auto currDev = qobject_cast<QSerialPort*>(ioDevice_); currDev) {
            installed = currDev->setRequestToSend(rts);
        }
    }
}

void Link::setDataTerminalReady(bool dtr) {
    bool installed = false;
    if (linkType_ == LinkType::kLinkSerial) {
        if (auto currDev = qobject_cast<QSerialPort*>(ioDevice_); currDev) {
            installed = currDev->setDataTerminalReady(dtr);
        }
    }
}

void Link::setParity(bool parity)
{
    parity_ = parity;
    bool installed = false;
    if (linkType_ == LinkType::kLinkSerial) {
        if (auto currDev = qobject_cast<QSerialPort*>(ioDevice_); currDev) {
            if(parity_) {
                currDev->setParity(QSerialPort::EvenParity);
            } else {
                currDev->setParity(QSerialPort::NoParity);
            }
        }
    }
}

void Link::setLinkType(LinkType linkType)
{
    linkType_ = linkType;
}

void Link::setAddress(const QString &address)
{
    address_ = address;
}

void Link::setSourcePort(int sourcePort)
{
    sourcePort_ = sourcePort;
}

void Link::setDestinationPort(int destinationPort)
{
    destinationPort_ = destinationPort;
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
    resetLastSearchIndx();
}

void Link::setIsUpgradingState(bool state)
{
    if (onUpgradingFirmware_ != state) {
        onUpgradingFirmware_ = state;
        emit upgradingFirmwareStateChanged(uuid_);
    }
}

void Link::setAutoConnOnce(bool state)
{
    autoConnOnce_ = state;
}

QUuid Link::getUuid() const
{
    return uuid_;
}

bool Link::getConnectionStatus() const
{
    if (ioDevice_) {
        if (ioDevice_->isOpen()) {
            return true;
        }
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

bool Link::getIsUpgradingState() const
{
    return onUpgradingFirmware_;
}

bool Link::getAutoConnOnce() const
{
    return autoConnOnce_;
}

void Link::setAttribute(LinkAttribute attribute) {
    attribute_ = attribute;
}

bool Link::writeFrame(FrameParser frame)
{
    return  attribute_ == LinkAttribute::kLinkAttributeNone && frame.isComplete() && write(QByteArray((const char*)frame.frame(), frame.frameLen()));
}

bool Link::write(QByteArray data)
{
    if (!ioDevice_ || !isOpen()) {
        return false;
    }

    switch (linkType_) {
        case LinkType::kLinkIPUDP: {
            auto udp = qobject_cast<QUdpSocket*>(ioDevice_);
            udp->writeDatagram(data, hostAddress_, destinationPort_);
            break;
        }
        case LinkType::kLinkIPTCP: {
            ioDevice_->write(data);
            break;
        }
        case LinkType::kLinkSerial: {
            ioDevice_->write(data);
            break;
        }

        default:
            return false;
    }

    return true;
}

void Link::onStartUpgradingFirmware()
{
    qDebug() << "ON START UPGRADING FIRMWARE" << uuid_;

    setIsUpgradingState(true);
    timeoutCnt_ = linkNumTimeoutsSmall;
    requestCnt_ = requestAllCntSmall;
    resetLastSearchIndx();
    setAutoConnOnce(true); // logger
}

void Link::onUpgradingFirmwareDone()
{
    qDebug() << "ON UPGRADING FIRMWARE DONE !" << uuid_;

    setIsUpgradingState(false);
    timeoutCnt_ = linkNumTimeoutsSmall;
    localGhostIgnoreCount_ = ghostIgnoreCount;
    requestCnt_ = requestAllCntBig;
    resetLastSearchIndx();
    setAutoConnOnce(true); // logger
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
        if (localGhostIgnoreCount_ > 0) {
            qDebug() << "Skipping ghost data after firmware..." << localGhostIgnoreCount_;
            --localGhostIgnoreCount_;
        }
        else {
            isReceivesData_ = true;
            timeoutCnt_ = linkNumTimeoutsBig;
            resetLastSearchIndx();
        }
    }
    else {
        if (timeoutCnt_ > 0) {
            --timeoutCnt_;
        }
        if (!timeoutCnt_ && isReceivesData_) {
            isReceivesData_ = false;
        }
    }

    // gui
    if (lastIsReceivesData != isReceivesData_) {
        emit isReceivesDataChanged(uuid_);
    }

    // autosearch
    bool isAutoSpeedSelection = autoSpeedSelection_ || (!autoSpeedSelection_ && onUpgradingFirmware_) || localGhostIgnoreCount_;
    bool isNeedSearch = isAutoSpeedSelection && !isReceivesData_ && !timeoutCnt_ && !baudrateSearchList_.empty();

    if (isNeedSearch) {
        timeoutCnt_ = linkNumTimeoutsSmall;
        //qDebug() << "   link: timeout ended do emit sendDoRequestAll" << uuid_;
        auto currBaudrate = baudrateSearchList_.at(lastSearchIndx_);
        lastSearchIndx_ = (lastSearchIndx_ + 1) % baudrateSearchList_.size();
        //qDebug() << "   link: trying find" << currBaudrate << "on" << lastSearchIndx_;
        setBaudrate(currBaudrate);
        emit baudrateChanged(uuid_);
    }

    if (!isReceivesData_ || !requestCnt_) {
        if(attribute_ == LinkAttribute::kLinkAttributeNone) {
            emit sendDoRequestAll(uuid_);
        }
        if (!requestCnt_) {
            requestCnt_ = onUpgradingFirmware_ ? requestAllCntSmall : isReceivesData_ ? requestAllCntBig : requestAllCntSmall;
        }
    }
    --requestCnt_;

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
    QPointer<QIODevice> dev = ioDevice_;
    if (!dev)
        return;

    if (dev->isOpen()) {
        dev->close();
        emit connectionStatusChanged(uuid_);
    }

    if (dev) {
        dev->disconnect(this);
        this->disconnect(dev);
    }

    if (dev)
        dev->deleteLater();

    ioDevice_.clear();
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

void Link::resetLastSearchIndx()
{
    lastSearchIndx_ = 0;
}

void Link::readyRead()
{
    auto dev = ioDevice_;
    if (!dev) {
        return;
    }

    if (linkType_ == LinkType::kLinkIPUDP) {
        auto socsUdp = qobject_cast<QUdpSocket*>(dev);

        while (socsUdp->hasPendingDatagrams()) {
            QByteArray datagram;
            datagram.resize(socsUdp->pendingDatagramSize());
            QHostAddress sender;
            quint16 senderPort;

            qint64 slen = socsUdp->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
            if (slen == -1)
                break;
            if(attribute_ == LinkAttribute::kLinkAttributeNone) {
                toParser(datagram);
            } else {
                emit dataReady(datagram);
            }
        }
    } else { // tcp, serial
        QByteArray data = dev->readAll();
        if (!data.isEmpty()) {
            if(attribute_ == LinkAttribute::kLinkAttributeNone) {
                toParser(data);
            } else {
                emit dataReady(data);
            }
        }
    }
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
