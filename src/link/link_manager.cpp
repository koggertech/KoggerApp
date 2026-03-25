#include "link_manager.h"

#include <QFile>
#include <QXmlStreamReader>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QRegularExpression>

namespace {

bool xmlBoolValue(const QString& value)
{
    const QString normalized = value.trimmed().toUpper();
    return normalized == QStringLiteral("TRUE") || normalized == QStringLiteral("1");
}

}


LinkManager::LinkManager(QObject *parent) :
    QObject(parent),
    coldStarted_(true)
{
    qRegisterMetaType<ControlType>("ControlType");
    qRegisterMetaType<LinkType>("LinkType");
    qRegisterMetaType<FrameParser>("FrameParser");
}

QList<QSerialPortInfo> LinkManager::getCurrentSerialList() const
{
    const auto allPorts = QSerialPortInfo::availablePorts();

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    QList<QSerialPortInfo> filteredPorts;
    for (const auto& portInfo : allPorts) {
        const QString systemLocation = portInfo.systemLocation();
        const bool hasUsbIdentifiers = portInfo.hasVendorIdentifier() || portInfo.hasProductIdentifier();
        const bool hasUsbLikeName = systemLocation.startsWith("/dev/ttyUSB")
                                 || systemLocation.startsWith("/dev/ttyACM");

        if (hasUsbIdentifiers || hasUsbLikeName) {
            filteredPorts.append(portInfo);
        }
    }
    return filteredPorts;
#else
    return allPorts;
#endif
}

Link* LinkManager::createSerialPort(const QSerialPortInfo &serialInfo) const
{
    Link* newLinkPtr = nullptr;

    if (serialInfo.isNull())
        return newLinkPtr;
    newLinkPtr = createNewLink();
    newLinkPtr->createAsSerial(serialInfo.portName(), 921600, false);   

    return newLinkPtr;
}

void LinkManager::addNewLinks(const QList<QSerialPortInfo> &currSerialList)
{
    for (const auto& itmI : currSerialList) {
        bool isBeen{ false };

        for (auto& itmJ : list_) {
            if (itmJ->getLinkType() != LinkType::kLinkSerial)
                continue;

            if (itmI.portName() == itmJ->getPortName()) {
                isBeen = true;
                break;
            }
        }

        if (!isBeen) {
            auto link = createSerialPort(itmI);
            list_.append(link);
            doEmitAppendModifyModel(link);
        }
    }
}

void LinkManager::deleteMissingLinks(const QList<QSerialPortInfo> &currSerialList)
{
    for (int i = 0; i < list_.size(); ++i) {
        Link* link = list_.at(i);

        if (link->getLinkType() != LinkType::kLinkSerial) {
            continue;
        }
        if (link->getIsUpgradingState()) {
            continue;
        }

        bool isBeen{ false };
        for (const auto& itm : currSerialList) {
            if (itm.portName() == link->getPortName()) {
                isBeen = true;
                break;
            }
        }

        if (link->getIsPinned()) {
            if (!isBeen && !link->getIsNotAvailable()) {
                if (link->isOpen())
                    link->close();
                link->setIsNotAvailable(true);
                doEmitAppendModifyModel(link);
            }
            else if (isBeen && link->getIsNotAvailable()) {
                link->setIsNotAvailable(false);
                doEmitAppendModifyModel(link);
            }
        }
        else if (!isBeen) {
            deleteLink(link->getUuid());
        }
    }
}

void LinkManager::openAutoConnections()
{
    for (int i = 0; i < list_.size(); ++i) { // do not open auto conns when file is open
        if (list_.at(i)->getIsForceStopped()) {
            return;
        }
    }

    for (int i = 0; i < list_.size(); ++i) {
        Link* link = list_.at(i);

        if (!link->getConnectionStatus()) {
            bool autoConnOnce = link->getAutoConnOnce();

            if ((link->getControlType() == ControlType::kAuto &&
                !link->getIsNotAvailable()) ||
                autoConnOnce) {

                if (autoConnOnce) {
                    link->setAutoConnOnce(false);
                }

                switch (link->getLinkType()) {
                    case LinkType::kLinkNone:   { break; }
                    case LinkType::kLinkSerial: { link->openAsSerial(); break; }
                    case LinkType::kLinkIPUDP:  { link->openAsUdp(); break; }
                    case LinkType::kLinkIPTCP:  { link->openAsTcp(); break; }
                    default:                   { break; }
                }
            }
        }
    }
}

void LinkManager::update()
{
    auto currSerialList{ getCurrentSerialList() };

    addNewLinks(currSerialList);

    deleteMissingLinks(currSerialList);

    openAutoConnections();
}

Link* LinkManager::getLinkPtr(QUuid uuid)
{
    const TimerController timerGuard(timer_.get());

    Link* retVal{ nullptr };

    for (auto& itm : list_) {
        if (itm->getUuid() == uuid) {
            retVal = itm;
            break;
        }
    }

    return retVal;
}

void LinkManager::doEmitAppendModifyModel(Link* linkPtr)
{
    const TimerController timerGuard(timer_.get());

    emit appendModifyModel(linkPtr->getUuid(),
                           linkPtr->getConnectionStatus(),
                           linkPtr->getIsRecievesData(),
                           linkPtr->getControlType(),
                           linkPtr->getPortName(),
                           linkPtr->getBaudrate(),
                           linkPtr->getParity(),
                           linkPtr->getLinkType(),
                           linkPtr->getAddress(),
                           linkPtr->getSourcePort(),
                           linkPtr->getDestinationPort(),
                           linkPtr->getIsPinned(),
                           linkPtr->getIsHided(),
                           linkPtr->getIsNotAvailable(),
                           linkPtr->getAutoSpeedSelection(),
                           linkPtr->getIsUpgradingState());
}

void LinkManager::exportPinnedLinksToXML()
{
    const TimerController timerGuard(timer_.get());

    const QString filePath = pinnedLinksFilePath();

    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            return;
        }
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement("pinned_links");

    for (auto& itm : list_) {
        if (itm->getIsPinned()) {
            xmlWriter.writeStartElement("link");
            xmlWriter.writeTextElement("uuid", itm->getUuid().toString());
            xmlWriter.writeTextElement("control_type", QString::number(static_cast<int>(itm->getControlType())));
            xmlWriter.writeTextElement("port_name", itm->getPortName());
            xmlWriter.writeTextElement("baudrate", QString::number(itm->getBaudrate()));
            xmlWriter.writeTextElement("parity", QVariant(static_cast<bool>(itm->getParity())).toString());
            xmlWriter.writeTextElement("link_type", QString::number(static_cast<int>(itm->getLinkType())));
            xmlWriter.writeTextElement("address", itm->getAddress());
            xmlWriter.writeTextElement("source_port", QString::number(itm->getSourcePort()));
            xmlWriter.writeTextElement("destination_port", QString::number(itm->getDestinationPort()));
            xmlWriter.writeTextElement("is_pinned", QVariant(static_cast<bool>(itm->getIsPinned())).toString());
            xmlWriter.writeTextElement("is_hided", QVariant(static_cast<bool>(itm->getIsHided())).toString());
            xmlWriter.writeTextElement("is_not_available", QVariant(static_cast<bool>(itm->getIsNotAvailable())).toString());
            xmlWriter.writeTextElement("connection_status", QVariant(static_cast<bool>(itm->getConnectionStatus())).toString());
            xmlWriter.writeTextElement("auto_speed_selection", QVariant(static_cast<bool>(itm->getAutoSpeedSelection())).toString());
            xmlWriter.writeEndElement();
        }
    }

    xmlWriter.writeEndElement();
    xmlWriter.writeEndDocument();
    file.close();
}

QString LinkManager::pinnedLinksFilePath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
        + QStringLiteral("/pinned_links.xml");
}

bool LinkManager::parsePinnedLinksXmlData(const QByteArray& xmlData, QList<PinnedLinkRecord>* records, QString* error) const
{
    if (!records) {
        if (error) {
            *error = QStringLiteral("records output is null");
        }
        return false;
    }

    records->clear();

    if (xmlData.trimmed().isEmpty()) {
        return true;
    }

    QXmlStreamReader xmlReader(xmlData);

    while (!xmlReader.atEnd()) {
        const auto token = xmlReader.readNext();
        if (token != QXmlStreamReader::StartElement || xmlReader.name() != QStringLiteral("link")) {
            continue;
        }

        PinnedLinkRecord record;
        while (!(xmlReader.tokenType() == QXmlStreamReader::EndElement && xmlReader.name() == QStringLiteral("link"))) {
            xmlReader.readNext();
            if (xmlReader.atEnd()) {
                break;
            }

            if (xmlReader.tokenType() != QXmlStreamReader::StartElement) {
                continue;
            }

            const QString tag = xmlReader.name().toString();
            if (tag == QStringLiteral("uuid")) {
                record.uuid = QUuid(xmlReader.readElementText());
            }
            else if (tag == QStringLiteral("connection_status")) {
                record.connectionStatus = xmlBoolValue(xmlReader.readElementText());
            }
            else if (tag == QStringLiteral("control_type")) {
                record.controlType = static_cast<ControlType>(xmlReader.readElementText().toInt());
            }
            else if (tag == QStringLiteral("port_name")) {
                record.portName = xmlReader.readElementText();
            }
            else if (tag == QStringLiteral("baudrate")) {
                record.baudrate = xmlReader.readElementText().toInt();
            }
            else if (tag == QStringLiteral("parity")) {
                record.parity = xmlBoolValue(xmlReader.readElementText());
            }
            else if (tag == QStringLiteral("link_type")) {
                record.linkType = static_cast<LinkType>(xmlReader.readElementText().toInt());
            }
            else if (tag == QStringLiteral("address")) {
                record.address = xmlReader.readElementText();
            }
            else if (tag == QStringLiteral("source_port")) {
                record.sourcePort = xmlReader.readElementText().toInt();
            }
            else if (tag == QStringLiteral("destination_port")) {
                record.destinationPort = xmlReader.readElementText().toInt();
            }
            else if (tag == QStringLiteral("is_pinned")) {
                record.isPinned = xmlBoolValue(xmlReader.readElementText());
            }
            else if (tag == QStringLiteral("is_hided")) {
                record.isHided = xmlBoolValue(xmlReader.readElementText());
            }
            else if (tag == QStringLiteral("is_not_available")) {
                record.isNotAvailable = xmlBoolValue(xmlReader.readElementText());
            }
            else if (tag == QStringLiteral("auto_speed_selection")) {
                record.autoSpeedSelection = xmlBoolValue(xmlReader.readElementText());
            }
            else {
                xmlReader.skipCurrentElement();
            }
        }

        if (record.uuid.isNull()) {
            record.uuid = QUuid::createUuid();
        }
        records->append(record);
    }

    if (xmlReader.hasError()) {
        if (error) {
            *error = xmlReader.errorString();
        }
        records->clear();
        return false;
    }

    return true;
}

bool LinkManager::looksLikeSerialPortName(const QString& portName)
{
    const QString normalized = portName.trimmed();
    if (normalized.isEmpty()) {
        return false;
    }

    if (normalized.startsWith(QStringLiteral("/dev/tty"), Qt::CaseInsensitive) ||
        normalized.startsWith(QStringLiteral("/dev/cu"), Qt::CaseInsensitive)) {
        return true;
    }

    static const QRegularExpression comPortExpression(QStringLiteral(R"(^COM\d+$)"),
                                                      QRegularExpression::CaseInsensitiveOption);
    return comPortExpression.match(normalized).hasMatch();
}

void LinkManager::appendPinnedLinkRecords(const QList<PinnedLinkRecord>& records)
{
    for (const PinnedLinkRecord& record : records) {
        Link* link = createNewLink();
        link->setUuid(record.uuid);
        link->setControlType(record.controlType);
        link->setPortName(record.portName);
        link->setBaudrate(record.baudrate);
        link->setParity(record.parity);
        link->setLinkType(record.linkType);
        link->setAddress(record.address);
        link->setSourcePort(record.sourcePort);
        link->setDestinationPort(record.destinationPort);
        link->setIsPinned(true);
        link->setIsHided(record.isHided);
        link->setIsNotAvailable(record.isNotAvailable);
        link->setAutoSpeedSelection(record.autoSpeedSelection);
        link->setIsForceStopped(false);

        list_.append(link);
        doEmitAppendModifyModel(link);

        if (record.connectionStatus) {
            link->setConnectionStatus(true);
            doEmitAppendModifyModel(link);
        }
    }
}

bool LinkManager::reloadPinnedLinksFromXmlData(const QByteArray& xmlData,
                                               bool allowSerialLinks,
                                               int* skippedSerialLinks,
                                               QString* error)
{
    const TimerController timerGuard(timer_.get());
    if (skippedSerialLinks) {
        *skippedSerialLinks = 0;
    }

    QList<PinnedLinkRecord> records;
    QString parseError;
    if (!parsePinnedLinksXmlData(xmlData, &records, &parseError)) {
        if (error) {
            *error = parseError;
        }
        return false;
    }

    if (!allowSerialLinks) {
        QList<PinnedLinkRecord> filteredRecords;
        filteredRecords.reserve(records.size());
        int skippedCount = 0;

        for (const PinnedLinkRecord& record : records) {
            const bool isSerialByType = record.linkType == LinkType::kLinkSerial;
            const bool isSerialByPortName = looksLikeSerialPortName(record.portName);
            if (isSerialByType || isSerialByPortName) {
                ++skippedCount;
                continue;
            }

            filteredRecords.append(record);
        }

        records = filteredRecords;
        if (skippedSerialLinks) {
            *skippedSerialLinks = skippedCount;
        }
    }

    // Stop all currently active links before replacing pinned set.
    for (Link* link : list_) {
        if (!link) {
            continue;
        }
        if (link->isOpen()) {
            link->close();
        }
        doEmitAppendModifyModel(link);
    }

    // Remove all existing pinned links.
    for (int i = list_.size() - 1; i >= 0; --i) {
        Link* link = list_.at(i);
        if (!link || !link->getIsPinned()) {
            continue;
        }

        emit linkDeleted(link->getUuid(), link);
        emit deleteModel(link->getUuid());

        link->disconnect();
        this->disconnect(link);

        if (link->isOpen()) {
            link->close();
        }

        list_.removeAt(i);
        delete link;
    }

    appendPinnedLinkRecords(records);
    exportPinnedLinksToXML();
    coldStarted_ = false;
    return true;
}

Link *LinkManager::createNewLink() const
{
    Link* retVal = new Link();

    QObject::connect(retVal, &Link::connectionStatusChanged, this, &LinkManager::onLinkConnectionStatusChanged);
    QObject::connect(retVal, &Link::upgradingFirmwareStateChanged, this, &LinkManager::onUpgradingFirmwareStateChanged);
    QObject::connect(retVal, &Link::frameReady, this, &LinkManager::frameReady);
    QObject::connect(retVal, &Link::closed, this, &LinkManager::linkClosed);
    QObject::connect(retVal, &Link::opened, this, &LinkManager::linkOpened);
    QObject::connect(retVal, &Link::baudrateChanged, this, &LinkManager::onLinkIsReceivesDataChanged);
    QObject::connect(retVal, &Link::isReceivesDataChanged, this, &LinkManager::onLinkIsReceivesDataChanged);
    QObject::connect(retVal, &Link::sendDoRequestAll, this, &LinkManager::sendDoRequestAll);

    return retVal;
}

void LinkManager::printLinkDebugInfo(Link* link) const
{
    const TimerController timerGuard(timer_.get());

    if (!link)
        qDebug() << "\tlink is nullptr";
    else {
        qDebug() << QString("uuid: %1; controlType: %2; portName: %3; baudrate: %4; parity: %5; linkType: %6; address: %7; sourcePort: %8; destinationPort: %9; isPinned: %10; isHided: %11; isNotAvailable: %12; connectionStatus: %13")
                        .arg(link->getUuid().toString()).arg(static_cast<int>(link->getControlType())).arg(link->getPortName()).arg(link->getBaudrate()).arg(link->getParity())
                        .arg(static_cast<int>(link->getLinkType())).arg(link->getAddress()).arg(link->getSourcePort()).arg(link->getDestinationPort()).arg(link->getIsPinned())
                        .arg(link->getIsHided()).arg(link->getIsNotAvailable()).arg(link->getConnectionStatus());
    }
}

void LinkManager::importPinnedLinksFromXML()
{
    const TimerController timerGuard(timer_.get());

    QFile file(pinnedLinksFilePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QList<PinnedLinkRecord> records;
    QString error;
    if (!parsePinnedLinksXmlData(file.readAll(), &records, &error)) {
        qWarning() << "LinkManager::importPinnedLinksFromXML parse error:" << error;
        return;
    }

    appendPinnedLinkRecords(records);
}

void LinkManager::onLinkConnectionStatusChanged(QUuid uuid)
{
    const TimerController timerGuard(timer_.get());

    if (const auto linkPtr = getLinkPtr(uuid); linkPtr) {
        doEmitAppendModifyModel(linkPtr);

        if (linkPtr->getIsPinned() && linkPtr->getConnectionStatus()) {
            exportPinnedLinksToXML();
        }
    }
}

void LinkManager::onUpgradingFirmwareStateChanged(QUuid uuid)
{
    const TimerController timerGuard(timer_.get());

    if (const auto linkPtr = getLinkPtr(uuid); linkPtr) {
        doEmitAppendModifyModel(linkPtr);
    }
}

void LinkManager::onLinkBaudrateChanged(QUuid uuid)
{
    const TimerController timerGuard(timer_.get());

    if (const auto linkPtr = getLinkPtr(uuid); linkPtr) {
        doEmitAppendModifyModel(linkPtr);

        if (linkPtr->getIsPinned()) {
            exportPinnedLinksToXML();
        }
    }
}

void LinkManager::onLinkIsReceivesDataChanged(QUuid uuid)
{
    const TimerController timerGuard(timer_.get());

    if (const auto linkPtr = getLinkPtr(uuid); linkPtr) {
        doEmitAppendModifyModel(linkPtr);
    }
}

void LinkManager::createAndStartTimer()
{
    if (!timer_) {
        timer_ = std::make_unique<QTimer>(this);
        timer_->setInterval(timerInterval_);
        QObject::connect(timer_.get(), &QTimer::timeout, this, &LinkManager::onExpiredTimer, Qt::QueuedConnection);
    }

    timer_->start();
}

void LinkManager::stopTimer()
{
    if (timer_) {
        timer_->stop();
    }
}

void LinkManager::onExpiredTimer()
{
    if (coldStarted_) {
        importPinnedLinksFromXML();
        coldStarted_ = false;
    }
    update();

    if (timer_) {
        timer_->start();
    }
}

void LinkManager::openAsSerial(QUuid uuid, LinkAttribute attribute)
{
    const TimerController timerGuard(timer_.get());

    if (const auto linkPtr = getLinkPtr(uuid); linkPtr) {
        linkPtr->setAttribute(attribute);
        linkPtr->setIsForceStopped(false);
        linkPtr->openAsSerial();
    }
}

void LinkManager::openAsUdp(QUuid uuid, QString address, int sourcePort, int destinationPort, LinkAttribute attribute)
{
    const TimerController timerGuard(timer_.get());

    if (const auto linkPtr = getLinkPtr(uuid); linkPtr) {
        linkPtr->setAttribute(attribute);
        linkPtr->setIsForceStopped(false);
        linkPtr->updateUdpParameters(address, sourcePort, destinationPort);
        linkPtr->openAsUdp();

        doEmitAppendModifyModel(linkPtr); //
    }
}

void LinkManager::openAsTcp(QUuid uuid, QString address, int sourcePort, int destinationPort, LinkAttribute attribute)
{
    const TimerController timerGuard(timer_.get());

    if (const auto linkPtr = getLinkPtr(uuid); linkPtr) {
        linkPtr->setAttribute(attribute);
        linkPtr->setIsForceStopped(false);
        linkPtr->updateTcpParameters(address, sourcePort, destinationPort);
        linkPtr->openAsTcp();

        doEmitAppendModifyModel(linkPtr); //
    }
}

void LinkManager::closeLink(QUuid uuid)
{
    const TimerController timerGuard(timer_.get());

    if (const auto linkPtr = getLinkPtr(uuid); linkPtr) {
        if (linkPtr->getControlType() == ControlType::kAuto)
            linkPtr->setIsForceStopped(true);
        linkPtr->close();

        doEmitAppendModifyModel(linkPtr); //

        if (linkPtr->getIsPinned()) {
            exportPinnedLinksToXML();
        }
    }
}

void LinkManager::closeFLink(QUuid uuid)
{
    const TimerController timerGuard(timer_.get());

    if (const auto linkPtr = getLinkPtr(uuid); linkPtr) {
        linkPtr->setIsForceStopped(true);
        linkPtr->close();
        doEmitAppendModifyModel(linkPtr); //
    }
}

void LinkManager::deleteLink(QUuid uuid)
{
    const TimerController timerGuard(timer_.get());

    if (const auto linkPtr = getLinkPtr(uuid); linkPtr) {
        emit linkDeleted(linkPtr->getUuid(), linkPtr);

        emit deleteModel(linkPtr->getUuid());
        linkPtr->disconnect();
        this->disconnect(linkPtr);

        if (linkPtr->isOpen())
            linkPtr->close();

        auto linkType = linkPtr->getLinkType();

        list_.removeOne(linkPtr);
        delete linkPtr;

        // manual deleting
        if (linkType == LinkType::kLinkIPTCP ||
            linkType == LinkType::kLinkIPUDP)
            exportPinnedLinksToXML();
    }
}

void LinkManager::updateBaudrate(QUuid uuid, int baudrate)
{
    const TimerController timerGuard(timer_.get());

    if (const auto linkPtr = getLinkPtr(uuid); linkPtr) {
        linkPtr->setBaudrate(baudrate);

        doEmitAppendModifyModel(linkPtr); // why?

        if (linkPtr->getIsPinned())
            exportPinnedLinksToXML();
    }
}

void LinkManager::setRequestToSend(QUuid uuid, bool rts) {
    if (const auto linkPtr = getLinkPtr(uuid); linkPtr) {
        linkPtr->setRequestToSend(rts);
    }
}

void LinkManager::setDataTerminalReady(QUuid uuid, bool dtr) {
    if (const auto linkPtr = getLinkPtr(uuid); linkPtr) {
        linkPtr->setDataTerminalReady(dtr);
    }
}

void LinkManager::setParity(QUuid uuid, bool parity) {
    if (const auto linkPtr = getLinkPtr(uuid); linkPtr) {
        linkPtr->setParity(parity);
    }
}

void LinkManager::setAttribute(QUuid uuid, LinkAttribute attribute) {
    if (const auto linkPtr = getLinkPtr(uuid); linkPtr) {
        linkPtr->setAttribute(attribute);
    }
}

void LinkManager::updateAddress(QUuid uuid, const QString &address)
{
    const TimerController timerGuard(timer_.get());

    if (const auto linkPtr = getLinkPtr(uuid); linkPtr) {
        linkPtr->setAddress(address);

        //doEmitAppendModifyModel(linkPtr); // why not?
        if (linkPtr->getIsPinned())
            exportPinnedLinksToXML();
    }
}

void LinkManager::updateAutoSpeedSelection(QUuid uuid, bool state)
{
    const TimerController timerGuard(timer_.get());

    if (const auto linkPtr = getLinkPtr(uuid); linkPtr) {
        linkPtr->setAutoSpeedSelection(state);

        //doEmitAppendModifyModel(linkPtr); // why not?
        if (linkPtr->getIsPinned())
            exportPinnedLinksToXML();
    }
}

void LinkManager::updateSourcePort(QUuid uuid, int sourcePort)
{
    const TimerController timerGuard(timer_.get());

    if (const auto linkPtr = getLinkPtr(uuid); linkPtr) {
        linkPtr->setSourcePort(sourcePort);

        //doEmitAppendModifyModel(linkPtr); //
        if (linkPtr->getIsPinned())
            exportPinnedLinksToXML();
    }
}

void LinkManager::updateDestinationPort(QUuid uuid, int destinationPort)
{
    const TimerController timerGuard(timer_.get());

    if (const auto linkPtr = getLinkPtr(uuid); linkPtr) {
        linkPtr->setDestinationPort(destinationPort);

        //doEmitAppendModifyModel(linkPtr); //
        if (linkPtr->getIsPinned())
            exportPinnedLinksToXML();
    }
}

void LinkManager::updatePinnedState(QUuid uuid, bool state)
{
    const TimerController timerGuard(timer_.get());

    if (auto linkPtr = getLinkPtr(uuid); linkPtr) {
        linkPtr->setIsPinned(state);

        exportPinnedLinksToXML();
    }
}

void LinkManager::updateControlType(QUuid uuid, ControlType controlType)
{
    const TimerController timerGuard(timer_.get());

    if (auto linkPtr = getLinkPtr(uuid); linkPtr) {
        linkPtr->setControlType(controlType);

        if (linkPtr->getIsPinned())
            exportPinnedLinksToXML();
    }
}

void LinkManager::frameInput(Link *link, Parsers::FrameParser frame)
{
    Q_UNUSED(link);
    Q_UNUSED(frame);
}

void LinkManager::createAsUdp(QString address, int sourcePort, int destinationPort)
{
    const TimerController timerGuard(timer_.get());

    Link* newLinkPtr = createNewLink();
    newLinkPtr->createAsUdp(address, sourcePort, destinationPort);
    list_.append(newLinkPtr);

    doEmitAppendModifyModel(newLinkPtr);
}

void LinkManager::createAsTcp(QString address, int sourcePort, int destinationPort)
{
    const TimerController timerGuard(timer_.get());

    Link* newLinkPtr = createNewLink();
    newLinkPtr->createAsTcp(address, sourcePort, destinationPort);
    list_.append(newLinkPtr);

    doEmitAppendModifyModel(newLinkPtr);
}

void LinkManager::openFLinks()
{
    const TimerController timerGuard(timer_.get());

    for (auto& itm : list_) {
        if (itm->getIsForceStopped()) {
            itm->setIsForceStopped(false);

            switch (itm->getLinkType()) {
            case LinkType::kLinkSerial: {
                itm->openAsSerial();
                break;
            }
            case LinkType::kLinkIPTCP : {
                itm->openAsTcp();
                break;
            }
            case LinkType::kLinkIPUDP: {
                itm->openAsUdp();
                break;
            }
            default:
                break;
            }
        }
    }
}

void LinkManager::createAndOpenAsUdpProxy(QString address, int sourcePort, int destinationPort)
{
    const TimerController timerGuard(timer_.get());

    Link* newLinkPtr = createNewLink();
    newLinkPtr->createAsUdp(address, sourcePort, destinationPort);
    newLinkPtr->setIsProxy(true);
    newLinkPtr->setIsHided(true);
    proxyLinkUuid_ = newLinkPtr->getUuid();
    list_.append(newLinkPtr);

    newLinkPtr->openAsUdp();
}

void LinkManager::closeUdpProxy()
{
    if (proxyLinkUuid_ == QUuid())
        return;

    deleteLink(proxyLinkUuid_);
    proxyLinkUuid_ = QUuid();
}

QUuid LinkManager::getFirstOpend() {
    for (auto& itm : list_) {
        if (itm->isOpen()) {
            return itm->getUuid();
        }
    }
    return QUuid();
}

LinkManager::TimerController::TimerController(QTimer *timer) : timer_(timer)
{
    if (timer_) {
        timer->stop();
    }
}

LinkManager::TimerController::~TimerController()
{
    if (timer_) {
        timer_->start();
    }
}
