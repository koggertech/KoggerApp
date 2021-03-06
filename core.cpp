#include "core.h"

Core::Core() : QObject(),
    m_console(new Console()),
    m_connection(new Connection()),
    m_plot(new PlotCash)
{
    connect(&_devs, &Device::chartComplete, m_plot, &PlotCash::addChart);
    connect(&_devs, &Device::distComplete, m_plot, &PlotCash::addDist);
    connect(&_devs, &Device::positionComplete, m_plot, &PlotCash::addPosition);

    connect(&_devs, &Device::upgradeProgressChanged, this, &Core::upgradeChanged);
}

void Core::consoleProto(ProtoBin &parser, bool is_in) {
    QString str_mode;
    QString comment = "";

    switch (parser.type()) {
    case CONTENT:
        str_mode = "data";
        if(parser.resp()) {
            switch(parser.frame()[6]) {
            case respNone: comment = "[respNone]"; break;
            case respOk: comment = "[respOk]"; break;
            case respErrorCheck: comment = "[respErrorCheck]"; break;
            case respErrorPayload: comment = "[respErrorPayload]"; break;
            case respErrorID: comment = "[respErrorID]"; break;
            case respErrorVersion: comment = "[respErrorVersion]"; break;
            case respErrorType: comment = "[respErrorType]"; break;
            case respErrorKey: comment = "[respErrorKey]"; break;
            case respErrorRuntime: comment = "[respErrorRuntime]"; break;
            default:
                comment = QString("[resp %1]").arg((int)parser.frame()[6]);
                break;
            }
        } else {
            if(parser.id() == ID_EVENT) {
                comment = QString("Event ID %1").arg(*(uint32_t*)(&parser.frame()[10]));
            }
        }
        break;
    case SETTING:
        str_mode = "set";
        break;
    case GETTING:
        str_mode = "get";
        break;
    default:
        str_mode = "none";
        break;
    }

    QString str_dir;
    if(is_in) { str_dir = "in"; }
    else { str_dir = "out"; }

    QString str_data = QByteArray((char*)parser.frame(), parser.frameLen()).toHex();

    consoleInfo(QString("%1: id %2 v%3, %4, len %5; %6 [ %7 ]").arg(str_dir).arg(parser.id()).arg(parser.ver()).arg(str_mode).arg(parser.payloadLen()).arg(comment).arg(str_data));
}

QList<QSerialPortInfo> Core::availableSerial(){
    return m_connection->availableSerial();
}

QStringList Core::availableSerialName(){
    QStringList serialNameList;
    const QList<QSerialPortInfo> serialList = availableSerial();
    for (const auto& serial : serialList) {
        if (!serial.portName().startsWith(QStringLiteral("cu."), Qt::CaseInsensitive)) {
            serialNameList.append(serial.portName());
        }
    }
    return serialNameList;
}

bool Core::openConnectionAsSerial(const QString &name, int baudrate, bool mode) {
    closeConnection();
    devsConnection();

    m_connection->openSerial(name, baudrate, false);
    m_connection->setRTS(false); // power on

    return true;
}

bool Core::devsConnection() {
    connect(m_connection, &Connection::closedEvent, this, &Core::connectionChanged);
    connect(m_connection, &Connection::openedEvent, this, &Core::connectionChanged);

    connect(m_connection, &Connection::openedEvent, &_devs, &Device::startConnection);
    connect(m_connection, &Connection::receiveData, &_devs, &Device::putData);
    connect(&_devs, &Device::dataSend, m_connection, &Connection::sendData);
    connect(m_connection, &Connection::loggingStream, &_logger, &Logger::loggingStream);

    if(_isLogging) {
        _logger.startNewLog();
    }

    return true;
}

bool Core::openConnectionAsFile(const QString &name) {
    closeConnection();

    m_plot->resetDataset();
    connect(m_connection, &Connection::openedEvent, &_devs, &Device::startConnection);
    connect(m_connection, &Connection::receiveData, &_devs, &Device::putData);
    m_connection->openFile(name);

    return true;
}

bool Core::isOpenConnection() {
    return m_connection->isOpen();
}

bool Core::closeConnection() {
    m_connection->close();
    _devs.stopConnection();

    m_connection->disconnect(&_devs);
    _devs.disconnect(m_connection);

    m_connection->disconnect(this);
    this->disconnect(m_connection);

    m_connection->disconnect(&_logger);

#ifdef FLASHER
    m_connection->disconnect(&flasher);
    flasher.disconnect(m_connection);
    flasher.disconnect(this);
#endif

    _logger.stopLogging();

    return true;
}

bool Core::connectionBaudrate(int baudrate) {
    return m_connection->setBaudrate(baudrate);
}

bool Core::upgradeFW(const QString &name, QObject* dev) {
    QUrl url(name);
    QFile m_file;

    if(url.isLocalFile()) {
        m_file.setFileName(url.toLocalFile());
    } else {
        m_file.setFileName(name);
    }

    bool is_open = false;
    is_open = m_file.open(QIODevice::ReadOnly);

    if(is_open == false) {  return false;  }

    DevQProperty* dev_q = (DevQProperty*)(dev);
    dev_q->sendUpdateFW(m_file.readAll());
    setUpgradeBaudrate();

    return true;
}

void Core::upgradeChanged(int progress_status) {
    if(progress_status == DevDriver::successUpgrade) {
        restoreBaudrate();
    }
}

void Core::setLogging(bool is_logging) {
    if(m_connection->isOpen()) {
        if(isLogging() && !is_logging) {
            _logger.stopLogging();
        } else if(!isLogging() && is_logging) {
            _logger.startNewLog();
        }
    }
    _isLogging = is_logging;
}

bool Core::isLogging() {
    return _isLogging;
}

bool Core::exportPlotAsCVS() {
    _logger.creatExportStream();

    _logger.dataExport("Meas. number, Event timestamp, Event ID, Distance, Processing distance\n");

    int row_cnt = m_plot->poolSize();

    for(int i = 0; i < row_cnt; i++) {
        PoolDataset* dataset = m_plot->fromPool(i);
        QString row_data;
        row_data.append(QString("%1,").arg(i));

        if(dataset->eventAvail()) {
            row_data.append(QString("%1,%2,").arg(dataset->eventTimestamp()).arg(dataset->eventID()));
        } else {
            row_data.append(",,");
        }

        if(dataset->distAvail()) {
            row_data.append(QString("%1,").arg((float)dataset->distData()*0.001f));
        } else {
            row_data.append(",");
        }

        if(dataset->distProccesingAvail()) {
            row_data.append(QString("%1,").arg((float)dataset->distProccesing()*0.001f));
        } else {
            row_data.append(",");
        }

        row_data.append("\n");
        _logger.dataExport(row_data);
    }

    _logger.endExportStream();

    return true;
}

void Core::restoreBaudrate() {
    m_connection->setBaudrate(backupBaudrate);
}

void Core::setUpgradeBaudrate() {
    backupBaudrate = m_connection->baudrate();
    m_connection->setBaudrate(115200);
}

void Core::UILoad(QObject *object, const QUrl &url) {
    m_waterFall = object->findChild<WaterFall*>();
    m_waterFall->setPlot(m_plot);
}

void Core::closing() {
}
