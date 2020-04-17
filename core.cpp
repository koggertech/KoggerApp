#include "core.h"

Core::Core() : QObject(),
    m_console(new Console()),
    m_connection(new Connection()),
    dev_driver(new SonarDriver()),
    m_plot(new PlotCash)
{
    connect(m_connection, &Connection::closedEvent, this, &Core::connectionChanged);
    connect(m_connection, &Connection::openedEvent, this, &Core::connectionChanged);
    connect(m_connection, &Connection::receiveData, dev_driver, &SonarDriver::putData);
    connect(dev_driver, &SonarDriver::dataSend, m_connection, &Connection::sendData);

    connect(dev_driver, &SonarDriver::chartComplete, m_plot, &PlotCash::addData);
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

bool Core::openConnectionAsSerial(const QString &name, int baudrate) {
    m_connection->openSerial(name, baudrate);
    return true;
}

bool Core::openConnectionAsFile(const QString &name) {
    m_connection->openFile(name);
    return true;
}

bool Core::isOpenConnection() {
    return m_connection->isOpen();
}

bool Core::closeConnection() {
    m_connection->close();
    return true;
}

bool Core::upgradeFW(const QString &name) {
    QUrl url(name);
    QFile m_file;

    if(url.isLocalFile()) {
        m_file.setFileName(url.toLocalFile());
    } else {
        m_file.setFileName(name);
    }

    bool is_open = false;
    is_open = m_file.open(QIODevice::ReadOnly);

    if(is_open == false) {
        qInfo("Upgrade failed to open");
        return false;
    }

    dev_driver->sendUpdateFW(m_file.readAll());

    return true;
}

void Core::UILoad(QObject *object, const QUrl &url) {
    qInfo("UI is load");
    WaterFall* waterFall = object->findChild<WaterFall*>();
    waterFall->setPlot(m_plot);
}

void Core::closing() {
}
