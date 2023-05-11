#include "core.h"

Core::Core() : QObject(),
    m_console(new Console()),
    m_connection(new Connection()),
    m_plot(new PlotCash)
{
//    m_connection->moveToThread(&connectionThread);
//    connectionThread.start();

    connect(&_devs, &Device::chartComplete, m_plot, &PlotCash::addChart);
    connect(&_devs, &Device::iqComplete, m_plot, &PlotCash::addIQ);
    connect(&_devs, &Device::distComplete, m_plot, &PlotCash::addDist);
    connect(&_devs, &Device::attitudeComplete, m_plot, &PlotCash::addAtt);
    connect(&_devs, &Device::positionComplete, m_plot, &PlotCash::addPosition);
    connect(&_devs, &Device::dopplerBeamComlete, m_plot, &PlotCash::addDopplerBeam);
    connect(&_devs, &Device::dvlSolutionComplete, m_plot, &PlotCash::addDVLSolution);

    connect(&_devs, &Device::upgradeProgressChanged, this, &Core::upgradeChanged);
}

void Core::consoleProto(FrameParser &parser, bool is_in) {
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
    consoleInfo("Scaning serial ports...");
    QStringList serialNameList;
    const QList<QSerialPortInfo> serialList = availableSerial();
    consoleInfo((QString("Find serial ports: %1").arg(serialList.size())));
    for (const auto& serial : serialList) {
//        if (!serial.portName().startsWith(QStringLiteral("cu."), Qt::CaseInsensitive)) {
            serialNameList.append(serial.portName());
            consoleInfo("Find serial:" + serial.portName());
//        }
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
bool Core::openConnectionAsIP(const QString &address, const int port, bool is_tcp) {
    connect(m_connection, &Connection::closedEvent, this, &Core::connectionChanged);
    connect(m_connection, &Connection::openedEvent, this, &Core::connectionChanged);

    connect(m_connection, &Connection::openedEvent, &_devs, &Device::startConnection);
    connect(m_connection, &Connection::receiveData, &_devs, &Device::putData);
    connect(&_devs, &Device::dataSend, m_connection, &Connection::sendData);
    connect(m_connection, &Connection::loggingStream, &_logger, &Logger::loggingStream);
    m_connection->openIP(address, port, is_tcp);
    return false;
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

bool Core::openProxy(const QString &address, const int port, bool is_tcp) {
    return false;
}

bool Core::closeProxy() {
    return false;
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
//    setUpgradeBaudrate();

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

bool Core::exportPlotAsCVS(QString file_path) {
    QString export_file_name;
    if(m_connection->lastType() == Connection::ConnectionFile) {
        export_file_name = m_connection->lastFileName().section('/', -1).section('.', 0, 0);
    } else {
        export_file_name = QDateTime::currentDateTime().toString("yyyy.MM.dd_hh:mm:ss").replace(':', '.');
    }

    _logger.creatExportStream(file_path + "/" + export_file_name + ".csv");

//    int row_cnt = m_plot->poolSize();
//    for(int i = 0; i < row_cnt; i++) {
//        PoolDataset* dataset = m_plot->fromPool(i);
//        QString row_data;
//        QVector<int16_t> chart = dataset->chartData();

//        if(chart.size() > 100) {
//            for(int chart_i = 0; chart_i < chart.size(); chart_i++) {
//                row_data.append(QString("%1,").arg(chart[chart_i]));
//            }
//            row_data.remove(row_data.length() - 1, 1);
//            row_data.append("\n");
//            _logger.dataExport(row_data);
//        }
//    }

//    _logger.endExportStream();

    bool meas_nbr = true;
    bool event_id = true;
    bool rangefinder = true;
    bool bottom_depth = true;
    bool pos_lat_lon = true;
    bool pos_time = true;

    if(meas_nbr) {
        _logger.dataExport("Number,");
    }

    if(event_id) {
        _logger.dataExport("Event UNIX,");
        _logger.dataExport("Event timestamp,");
        _logger.dataExport("Event ID,");
    }

    if(rangefinder) {
        _logger.dataExport("Rangefinder,");
    }

    if(bottom_depth) {
        _logger.dataExport("Bottom depth,");
    }

    if(pos_lat_lon) {
        _logger.dataExport("Latitude,");
        _logger.dataExport("Longitude,");

        if(pos_time) {
            _logger.dataExport("GNSS UTC Date,");
            _logger.dataExport("GNSS UTC Time,");
        }
    }



    _logger.dataExport("\n");

    int row_cnt = m_plot->poolSize();

    int prev_timestamp = 0;
    int prev_unix = 0;
    int prev_event_id = 0;
    int prev_dist_proc = 0;
    double prev_lat = 0, prev_lon = 0;

    for(int i = 0; i < row_cnt; i++) {
        PoolDataset* dataset = m_plot->fromPool(i);
        QString row_data;

        if(meas_nbr) {
            row_data.append(QString("%1,").arg(i));
        }

        if(event_id) {
            if(dataset->eventAvail()) {
                prev_timestamp = dataset->eventTimestamp();
                prev_event_id = dataset->eventID();
                prev_unix = dataset->eventUnix();
            }
            row_data.append(QString("%1,%2,%3,").arg(prev_unix).arg(prev_timestamp).arg(prev_event_id));
        }

        if(rangefinder) {
            if(dataset->distAvail()) {
                row_data.append(QString("%1,").arg((float)dataset->distData()*0.001f));
            } else {
                row_data.append("0,");
            }
        }

        if(bottom_depth) {
            if(dataset->distProccesingAvail()) {
                prev_dist_proc = dataset->distProccesing();
            }
            row_data.append(QString("%1,").arg((float)(prev_dist_proc)*0.001f));
        }

        if(pos_lat_lon) {
            if(dataset->isPosAvail()) {
                prev_lat = dataset->lat();
                prev_lon = dataset->lon();
            }

            row_data.append(QString::number(prev_lat, 'f'));
            row_data.append(",");
            row_data.append(QString::number(prev_lon, 'f'));
            row_data.append(",");

            if(pos_time) {
                if(dataset->isPosAvail() && dataset->positionTimeUnix() != 0) {
                    QDateTime dt = QDateTime::fromTime_t(dataset->positionTimeUnix(), Qt::TimeSpec::UTC).addMSecs(dataset->positionTimeNano()/1000000);
                    row_data.append(dt.toString("yyyy-MM-dd"));
                    row_data.append(",");
                    row_data.append(dt.toString("hh:mm:ss.zzz"));
                    row_data.append(",");
                } else {
                    row_data.append(",");
                    row_data.append(",");
                }
            }
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
    _render = object->findChild<FboInSGRenderer*>();

    if(_render != NULL) {
        m_plot->set3DRender(_render);
    }

    m_waterFall = object->findChild<WaterFall*>();
    if(m_waterFall != NULL) {
        m_waterFall->setPlot(m_plot);
    }
}

void Core::closing() {
}
