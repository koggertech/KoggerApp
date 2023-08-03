#include "core.h"
#include <iomanip>
#include <ctime>

Core::Core() : QObject(),
    m_console(new Console()),
    m_connection(new Connection()),
    _dataset(new Dataset)
{
//    m_connection->moveToThread(&connectionThread);
//    connectionThread.start();

    connect(&_devs, &Device::chartComplete, _dataset, &Dataset::addChart);
    connect(&_devs, &Device::iqComplete, _dataset, &Dataset::addIQ);
    connect(&_devs, &Device::distComplete, _dataset, &Dataset::addDist);
    connect(&_devs, &Device::attitudeComplete, _dataset, &Dataset::addAtt);
    connect(&_devs, &Device::positionComplete, _dataset, &Dataset::addPosition);
    connect(&_devs, &Device::dopplerBeamComlete, _dataset, &Dataset::addDopplerBeam);
    connect(&_devs, &Device::dvlSolutionComplete, _dataset, &Dataset::addDVLSolution);

    connect(&_devs, &Device::upgradeProgressChanged, this, &Core::upgradeChanged);
}


void Core::createControllers()
{
    mpSettings3DController = std::make_shared <Q3DSettingsController> ();
}

void Core::createModels()
{
    mpScene3DModel = std::make_shared <Q3DSceneModel> ();

    mpSettings3DController->setModel(mpScene3DModel);
    _dataset->set3DSceneModel(mpScene3DModel);
}

void Core::setEngine(QQmlApplicationEngine *engine)
{
    m_engine = engine;

    createControllers();
    createModels();

    if(mpSettings3DController) {
        m_engine->rootContext()->setContextProperty("Settings3DController", mpSettings3DController.get());
    }

    if(mpScene3DModel) {
        m_engine->rootContext()->setContextProperty("Scene3DModel", mpScene3DModel.get());
    }

}

void Core::consoleProto(FrameParser &parser, bool is_in) {
    QString str_mode;
    QString comment = "";

    switch (parser.type()) {
    case CONTENT:
        str_mode = "DATA";
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
        str_mode = "SET";
        break;
    case GETTING:
        str_mode = "GET";
        break;
    default:
        str_mode = "NAN";
        break;
    }

    QString str_dir;
    if(is_in) { str_dir = "-->> "; }
    else { str_dir = "<<-- "; }

    try {
        QString str_data = QByteArray((char*)parser.frame(), parser.frameLen()).toHex();

        consoleInfo(QString("%1KG[%2]: id %3 v%4, %5, len %6; %7 [ %8 ]").arg(str_dir).arg(parser.route()).arg(parser.id()).arg(parser.ver()).arg(str_mode).arg(parser.payloadLen()).arg(comment).arg(str_data));

    }catch(std::bad_alloc& ex){
        qCritical().noquote() << __func__ << " --> " << ex.what();
    }
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

bool Core::openConnectionAsSerial(const int id, bool autoconn, const QString &name, int baudrate, bool mode) {
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

bool Core::openConnectionAsFile(const int id, const QString &name, bool is_append) {
    closeConnection();
    if(mpScene3DModel){
        mpScene3DModel->clear();
    }

    if(!is_append) {
        _dataset->resetDataset();
    }

    QStringList splitname = name.split(QLatin1Char('.'), Qt::SkipEmptyParts);
    if(splitname.size() > 1) {
        QString format = splitname.last();
        if(format.contains("xtf", Qt::CaseInsensitive)) {

            QFile file;
            QUrl url(name);
            if(url.isLocalFile()) {
                file.setFileName(url.toLocalFile());
            } else {
                file.setFileName(url.toString());
            }

            if(file.open(QIODevice::ReadOnly)) {
                return openXTF(file.readAll());
            }

            return false;
        }
    }

    connect(m_connection, &Connection::openedEvent, &_devs, &Device::startConnection);
    connect(m_connection, &Connection::receiveData, &_devs, &Device::putData);
    m_connection->openFile(name);


    QList<DatasetChannel> chs = _dataset->channelsList();


    for(int i = 0; i < _plots2d.size(); i++) {
        if(i == 0 &&_plots2d.at(i) != NULL) {
            if(chs.size() >= 2) {
                _plots2d.at(i)->setDataChannel(chs[0].channel, chs[1].channel);
            }

            if(chs.size() == 1) {
                _plots2d.at(i)->setDataChannel(chs[0].channel);
            }
        }
//        if(_plots2d.at(i) != NULL && i < chs.size()) {
//            _plots2d.at(i)->setDataChannel(chs[i].channel);
//        }
    }

    return true;

}
bool Core::openConnectionAsIP(const int id, bool autoconn, const QString &address, const int port, bool is_tcp) {
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

bool Core::exportPlotAsCVS(QString file_path, int channel) {
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

    bool external_pos_lla = true;
    bool external_pos_neu = true;

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

    if(external_pos_lla) {
        _logger.dataExport("ExtLatitude,");
        _logger.dataExport("ExtLongitude,");
        _logger.dataExport("ExtAltitude,");
    }

    if(external_pos_neu) {
        _logger.dataExport("ExtNorth,");
        _logger.dataExport("ExtEast,");
        _logger.dataExport("ExtHeight,");
    }



    _logger.dataExport("\n");

    int row_cnt = _dataset->size();

    int prev_timestamp = 0;
    int prev_unix = 0;
    int prev_event_id = 0;
    float prev_dist_proc = 0;
    double prev_lat = 0, prev_lon = 0;

    for(int i = 0; i < row_cnt; i++) {
        Epoch* epoch = _dataset->fromIndex(i);
        QString row_data;

        if(meas_nbr) {
            row_data.append(QString("%1,").arg(i));
        }

        if(event_id) {
            if(epoch->eventAvail()) {
                prev_timestamp = epoch->eventTimestamp();
                prev_event_id = epoch->eventID();
                prev_unix = epoch->eventUnix();
            }
            row_data.append(QString("%1,%2,%3,").arg(prev_unix).arg(prev_timestamp).arg(prev_event_id));
        }

        if(rangefinder) {
            if(epoch->distAvail()) {
                row_data.append(QString("%1,").arg((float)epoch->distData()*0.001f));
            } else {
                row_data.append("0,");
            }
        }

        if(bottom_depth) {
            prev_dist_proc = epoch->distProccesing(channel);
            row_data.append(QString("%1,").arg((float)(prev_dist_proc)));
        }

        if(pos_lat_lon) {
            if(epoch->isPosAvail()) {
                prev_lat = epoch->lat();
                prev_lon = epoch->lon();
            }

            row_data.append(QString::number(prev_lat, 'f'));
            row_data.append(",");
            row_data.append(QString::number(prev_lon, 'f'));
            row_data.append(",");

            if(pos_time) {
                if(epoch->isPosAvail() && epoch->positionTimeUnix() != 0) {
                    QDateTime dt = QDateTime::fromTime_t(epoch->positionTimeUnix(), Qt::TimeSpec::UTC).addMSecs(epoch->positionTimeNano()/1000000);
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

        Position position = epoch->getExternalPosition();

        if(external_pos_lla) {
            row_data.append(QString::number(position.lla.latitude, 'f', 10));
            row_data.append(",");
            row_data.append(QString::number(position.lla.longitude, 'f', 10));
            row_data.append(",");
            row_data.append(QString::number(position.lla.altitude, 'f', 10));
            row_data.append(",");
        }

        if(external_pos_neu) {
            row_data.append(QString::number(position.ned.n, 'f', 10));
            row_data.append(",");
            row_data.append(QString::number(position.ned.e, 'f', 10));
            row_data.append(",");
            row_data.append(QString::number(-position.ned.d, 'f', 10));
            row_data.append(",");
        }


        row_data.append("\n");
        _logger.dataExport(row_data);
    }

    _logger.endExportStream();

    return true;
}

bool Core::exportPlotAsXTF(QString file_path) {
    QString export_file_name;
    if(m_connection->lastType() == Connection::ConnectionFile) {
        export_file_name = m_connection->lastFileName().section('/', -1).section('.', 0, 0);
    } else {
        export_file_name = QDateTime::currentDateTime().toString("yyyy.MM.dd_hh:mm:ss").replace(':', '.');
    }

    _logger.creatExportStream(file_path + "/_" + export_file_name + ".xtf");


    QList<DatasetChannel> chs = _dataset->channelsList();
    QByteArray data_export = _converterXTF.toXTF(dataset(), chs[0].channel, chs[1].channel);

    _logger.dataByteExport(data_export);

//    _logger.dataByteExport(_converterXTF._lastData);

    _logger.endExportStream();
    return true;
}

bool Core::openXTF(QByteArray data) {
    _dataset->resetDataset();

    _converterXTF.toDataset(data, dataset());

    consoleInfo("XTF note:" + QString(_converterXTF.header.NoteString));
    consoleInfo("XTF programm name:" + QString(_converterXTF.header.RecordingProgramName));
    consoleInfo("XTF sonar name:" + QString(_converterXTF.header.SonarName));

    QList<DatasetChannel> chs = _dataset->channelsList();

    for(int i = 0; i < _plots2d.size(); i++) {
        if(_plots2d.at(i) != NULL && i < chs.size()) {
            if(i == 0) {
                _plots2d.at(i)->setDistance(-300, 300);
                _plots2d.at(i)->setDataChannel(chs[0].channel, chs[1].channel);
            }

//            if(i == 1) {
//                _plots2d.at(i)->setDistance(0, 100);
//            }
//            _plots2d.at(i)->setDataChannel(chs[i].channel);
        }
    }

    return true;
}

bool Core::openCSV(QString name, int separator_type, QString time_format, int first_row, int col_time, int col_lat, int col_lon, int col_altitude, int col_north, int col_east, int col_up) {

    QFile file;
    QUrl url(name);
    if(url.isLocalFile()) {
        file.setFileName(url.toLocalFile());
    } else {
        file.setFileName(url.toString());
    }

    if(!file.open(QIODevice::ReadOnly)) {
        return false;
    }


    QString separator("");
    switch(separator_type) {
        case 0: separator = ","; break;
        case 1: separator = "	"; break;
        case 2: separator = " "; break;
        case 3: separator = ";"; break;
    default: separator = QString(separator_type);
    }

    QList<Position> track;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString row = in.readLine();
        if(row[0] == '%' || row[0] == '#') {
            continue;
        }

        QStringList columns = row.split(separator);

        track.append(Position());

        if(col_time > 0 && col_time-1 < columns.size()) {
            QDateTime time = QDateTime::fromString(columns[col_time-1], time_format);
            if(time.isValid()) {
                time.setTimeSpec(Qt::UTC);
                int64_t unix_msec = time.toMSecsSinceEpoch();
                track.last().time.unix = unix_msec/1000;
                track.last().time.nanoSec = (unix_msec%1000)*1e6;
            }
        }

        if(col_lat > 0 && col_lat-1 < columns.size()) {
            track.last().lla.latitude = columns[col_lat-1].replace(QLatin1Char(','), QLatin1Char('.')).toDouble();
        }

        if(col_lon > 0 && col_lon-1 < columns.size()) {
            track.last().lla.longitude = columns[col_lon-1].replace(QLatin1Char(','), QLatin1Char('.')).toDouble();
        }

        if(col_altitude > 0 && col_altitude-1 < columns.size()) {
            track.last().lla.altitude = columns[col_altitude-1].replace(QLatin1Char(','), QLatin1Char('.')).toDouble();
        }

        if(col_north > 0 && col_north-1 < columns.size()) {
            track.last().ned.n = columns[col_north-1].replace(QLatin1Char(','), QLatin1Char('.')).toDouble();
        }

        if(col_east > 0 && col_east-1 < columns.size()) {
            track.last().ned.e = columns[col_east-1].replace(QLatin1Char(','), QLatin1Char('.')).toDouble();
        }

        if(col_up > 0 && col_up-1 < columns.size()) {
            track.last().ned.d = -columns[col_up-1].replace(QLatin1Char(','), QLatin1Char('.')).toDouble();;
        }
    }

    _dataset->mergeGnssTrack(track);

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
        _dataset->set3DRender(_render);
    }

//    m_waterFall = object->findChild<WaterFall*>();
//    if(m_waterFall != NULL) {
//        m_waterFall->setPlot(m_plot);
//    }

    _plots2d = object->findChildren<qPlot2D*>();

    for(int i = 0; i < _plots2d.size(); i++) {
        if(_plots2d.at(i) != NULL) {
            _plots2d.at(i)->setPlot(_dataset);
        }
    }
}

void Core::closing() {
}
