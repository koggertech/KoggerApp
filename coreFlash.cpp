#include "core.h"
#include "QThread"

#ifdef FLASHER
bool Core::factoryFlash(const QString &name, int sn, QString pn) {
    if(m_connection->isOpen()) {
        m_connection->disconnect(&_devs);
        m_connection->disconnect(&flasher);
        _devs.disconnect(m_connection);
        flasher.disconnect(m_connection);


        flasher.disconnect(this);

        connect(m_connection, &Connection::openedEvent, &flasher, &Flasher::startConnection);
        connect(m_connection, &Connection::receiveData, &flasher, &Flasher::putData);
        connect(&flasher, &Flasher::connectionChanged, this, &Core::flasherConnectionChanged);
        connect(&flasher, &Flasher::dataSend, m_connection, &Connection::sendData);

        backupBaudrate = m_connection->baudrate();

        consoleInfo("Port reconnection to Flasher");
    } else {
        return false;
    }


    m_connection->openSerial(115200, true);

    if(m_connection->isOpen()) {
        m_connection->setRTS(true); // power off
        QThread::msleep(500);
        m_connection->setDTR(true); // booster on
        QThread::msleep(50);
        m_connection->setRTS(false); // power on
        QThread::msleep(100);
        m_connection->setDTR(false); // booster off
    } else {
        consoleInfo("Core: port for flasher not avail");
        return false;
    }

    QUrl url(name);
    QFile m_file;

    if(url.isLocalFile()) { m_file.setFileName(url.toLocalFile());
    } else { m_file.setFileName(name); }

    bool is_file_open = false;
    is_file_open = m_file.open(QIODevice::ReadOnly);
    if(is_file_open == false) {
        consoleInfo("FW file isn't open");
        return false;
    }

    fw_data = m_file.readAll();
    uint8_t* fw_ptr = (uint8_t*)fw_data.data();
    // date
    fw_ptr[456] = 0;
    fw_ptr[457] = 0;
    // id
    uint16_t f_id = 123;
    uint8_t* f_id_ptr = (uint8_t*)(&f_id);
    fw_ptr[458] = f_id_ptr[0];
    fw_ptr[459] = f_id_ptr[1];
    // sn
    uint8_t* sn_ptr = (uint8_t*)(&sn);
    fw_ptr[460] = sn_ptr[0];
    fw_ptr[461] = sn_ptr[1];
    fw_ptr[462] = sn_ptr[2];
    fw_ptr[463] = sn_ptr[3];

    for(int i = 0; i < 16; i++){
        if(i < pn.size()) {
            fw_ptr[464 + i] = pn.toStdString()[i];
        } else {
             fw_ptr[464 + i] = pn.toStdString()[i];
        }
    }

    return true;
}

void Core::flasherConnectionChanged(Flasher::BootState connection_status) {
    QThread::msleep(100);
    if(connection_status == Flasher::BootReady) {
        flasher.erase();
    } else if(connection_status == Flasher::BootLock) {
        flasher.unprotect();
    } else if(connection_status == Flasher::BootErase) {
        if(fw_data.length() > 0) {
            consoleInfo("Core: start flash");
            flasher.write(0x08000000, fw_data);
        }
    } else if(connection_status == Flasher::BootUnLock || connection_status == Flasher::BootUnLockFail) {
        consoleInfo("Core: start after unlock process");
        m_connection->setRTS(true); // power off
        QThread::msleep(200);
        m_connection->setDTR(true); // booster on
        QThread::msleep(20);
        m_connection->setRTS(false); // power on
        QThread::msleep(100);
        m_connection->setDTR(false); // booster off
        flasher.startConnection(true);
    } else if(connection_status == Flasher::BootWriteSuccess) {
        flasher.read(0x08000000, fw_data.length());
    } else if(connection_status == Flasher::BootAsk) {
        flasher.read(0x08000000, 4);
    } else if(connection_status == Flasher::BootReadSuccess) {
        if(flasher.check(fw_data)) {
            m_connection->setRTS(true); // power off
            QThread::msleep(500);
            m_connection->setRTS(false); // power on
            QThread::msleep(500);

            closeConnection();
            devsConnection();
            m_connection->openSerial(backupBaudrate, false);
        }
    }
}
#endif
