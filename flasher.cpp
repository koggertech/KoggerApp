#include "flasher.h"

#include <core.h>
extern Core core;

Flasher::Flasher(QObject *parent) : QObject(parent) {

}

void Flasher::putData(const QByteArray &data) {
    m_cmdRx.append(data);
    uint32_t rx_len = m_cmdRx.length();
    uint8_t* rx_data = (uint8_t*)(m_cmdRx.data());
//    QString str_data = QByteArray((char*)rx_data, rx_len).toHex(' ');
//    core.consoleInfo(QString(">> arm: rx CMD %1, state %2, len %3, rx %4").arg(lastCMD).arg(m_state).arg(rx_len).arg(str_data));

    if(m_state == CmdSend) {
        if(rx_len > 0 && rx_data[0] == ACK) {
            if(lastCMD == CMD_VER_RDP && rx_len == 5) {
                if(rx_data[4] == ACK) {
                    bootVer = rx_data[1];
                    OB[0] = rx_data[2];
                    OB[1] = rx_data[3];
                    core.consoleInfo(QString(">> arm: ver %1.%2, OB %3 %4").arg(bootVer>>4).arg(bootVer&0xF).arg(OB[0]).arg(OB[1]));
                } else {
                    m_state = RxNAsk;
                }
            } else if(lastCMD == CMD_ReadMemory) {
                switch (m_readState) {
                case readIdle:
                    m_readState = readCmdAsk;
                    sendAddr(m_readAddr);
                    break;
                case readCmdAsk:
                    m_readState = readAddrAsk;
                    m_readLastLen = m_readAddrEnd - m_readAddr;
                    if(m_readLastLen > 256) m_readLastLen = 256;
                    sendLen(m_readLastLen - 1);
                    m_readAddr += m_readLastLen;
                    break;
                case readAddrAsk:
                    if(m_readLastLen <= rx_len - 1) {
                        m_readState = readIdle;
                        m_cmdRx.remove(0, m_readLastLen - rx_len);
                        m_readBuf.append(m_cmdRx.right(m_readLastLen));
//                        core.consoleInfo(QString("arm: complete read addr %1, len %2, addrEnd %3").arg(m_readAddr).arg(m_readLastLen).arg(m_readAddrEnd));

                        if((m_readBuf.length())*100/m_readMax != m_readProgress) {
                            m_readProgress = (m_readBuf.length())*100/m_readMax;
                            emit readProgressChanged();
                        }


                        if(m_readAddr < m_readAddrEnd) {
                            sendCMD(CMD_ReadMemory);
                        } else {
                            if(m_testRDP) {
                                core.consoleInfo("arm: read success");
                                emit connectionChanged(BootReadSuccess);
                            } else {
                                m_testRDP = true;
                                core.consoleInfo("arm: boot ready");
                                emit connectionChanged(BootReady);
                            }
                        }
                    }
                    break;
                }
            } else if(lastCMD == CMD_WriteMemory) {
                switch (m_writeState) {
                case writeIdle:
                    m_writeState = writeCmdAsk;
                    sendAddr(m_writeAddr);
                    break;
                case writeCmdAsk:
                    m_writeLen = m_writeBuf.length();
                    if(m_writeLen > 256) m_writeLen = 256;
                    writeData(m_writeBuf.left(m_writeLen));
                    m_writeBuf.remove(0, m_writeLen);
                    m_writeAddr += m_writeLen;
                    if((m_writeMax - m_writeBuf.length())*100/m_writeMax != m_writeProgress) {
                        m_writeProgress = (m_writeMax - m_writeBuf.length())*100/m_writeMax;
                        emit writeProgressChanged();
                    }

                    m_writeState = writeDataAsk;
                    break;
                case writeDataAsk:
                    m_writeState = writeIdle;

                    if(m_writeBuf.length() > 0) {
                        sendCMD(CMD_WriteMemory);
                    } else {
                        core.consoleInfo("arm: write success");
                        emit connectionChanged(BootWriteSuccess);
                    }
                    break;
                }

            } else if(lastCMD == CMD_Connect && rx_len >= 1) {
                m_connect = true;
                core.consoleInfo("arm: boot ask");
                emit connectionChanged(BootAsk);

            } else if(lastCMD == CMD_Unlock && rx_len >= 2) {
                if(rx_data[0] == ACK && rx_data[1] == ACK) {
                    core.consoleInfo("arm: unlock success");
                    emit connectionChanged(BootUnLock);
                } else {
                    core.consoleInfo("arm: unlock fail");
                }
            } else if(lastCMD == CMD_ExtErase) {
                if(rx_data[0] == ACK && rx_len == 1) {
                    QByteArray data;
                    data.append(0xFF);
                    data.append(0xFF);
                    data.append(char(0));
                    dataSend(data);
                    core.consoleInfo("arm: mass erase start");
                } else if( rx_data[0] == ACK && rx_data[1] == ACK) {
                    core.consoleInfo("arm: mass erase done");
                    emit connectionChanged(BootErase);
                }

            } else {
                core.consoleInfo("arm: undef last CMD beh");
            }
        } else if(rx_len > 0 && rx_data[0] == NACK) {
//            m_state = RxNAsk;
            core.consoleInfo("arm: NASK");

            if(lastCMD == CMD_ReadMemory) {
                core.consoleInfo("arm: boot has lock");
                emit connectionChanged(BootLock);
            } else if(lastCMD == CMD_Unlock) {
                core.consoleInfo("arm: unlock fail");
                unprotect();
//                emit connectionChanged(BootUnLockFail);
            }
        } else {

            core.consoleInfo("arm: wait CMD");
        }

    } else {
        m_state = Undef;
    }
}

void Flasher::getVerAndRDP() {
    sendCMD(CMD_VER_RDP);
}

void Flasher::read(uint32_t addr, uint32_t size) {
    m_checkProgress = 0;
    emit checkProgressChanged();

    m_readMax = size;
    m_readProgress = 0;
    emit readProgressChanged();
    m_readBuf.clear();
    m_readState = readIdle;
    m_readAddr = addr;
    m_readAddrEnd = m_readAddr + size;
    sendCMD(CMD_ReadMemory);
}

void Flasher::write(uint32_t addr, const QByteArray &data) {
    m_checkProgress = 0;
    emit checkProgressChanged();

    m_writeBuf.clear();
    m_writeBuf.append(data);
    m_writeMax = m_writeBuf.length();
    m_writeProgress = 0;
    emit writeProgressChanged();
    m_writeState = writeIdle;
    m_writeAddr = addr;
    sendCMD(CMD_WriteMemory);
}

void Flasher::unprotect() {
    sendCMD(CMD_Unlock);
}

void Flasher::erase() {
    m_checkProgress = 0;
    emit checkProgressChanged();
    core.consoleInfo("arm: try erase");
    sendCMD(CMD_ExtErase);
}

bool Flasher::check(QByteArray &data) {
    QByteArray fw_read = getRead();
    char* data_read = fw_read.data();
    char* data_write = data.data();
    bool equal = fw_read.length() == data.length();

    for(int i = 0; i < fw_read.length(); i++ && equal) {
        if(data_read[i] != data_write[i]) {  equal = false; }
    }

    if(equal) { m_checkProgress = 100;
    } else { m_checkProgress = 0; }
    emit checkProgressChanged();

    return equal;
}

void Flasher::startConnection(bool duplex) {
    m_cmdRx.clear();

    m_state = CmdSend;
    lastCMD = CMD_Connect;
    m_connect = false;
    m_testRDP = false;
    unlockCnt = 0;

    m_checkProgress = 0;
    emit checkProgressChanged();

    QByteArray data;
    data.append(0x7F);
    dataSend(data);

    core.consoleInfo("arm: get boot mode");
}

bool Flasher::sendCMD(CMD_ID id) {
    m_cmdRx.clear();

    QByteArray data;
    data.append(id);
    data.append((~id)&0xFF);
    dataSend(data);
    m_state = CmdSend;
    lastCMD = id;
//    QString str_data = QByteArray(data.data(), data.length()).toHex(' ');
//    core.consoleInfo(QString("<< arm: send CMD %1").arg(id));

    return true;
}

bool Flasher::sendAddr(uint32_t addr) {
    m_cmdRx.clear();
    char* addr_ptr = (char*)(&addr);
    QByteArray data;
    data.append(addr_ptr[3]);
    data.append(addr_ptr[2]);
    data.append(addr_ptr[1]);
    data.append(addr_ptr[0]);
    data.append(addr_ptr[0] ^ addr_ptr[1] ^ addr_ptr[2] ^ addr_ptr[3]);

//    QString str_data = QByteArray(data.data(), data.length()).toHex(' ');
//    core.consoleInfo(QString("<< arm: send ADDR %1").arg(str_data));
    dataSend(data);
    m_state = CmdSend;
    return true;
}

bool Flasher::sendLen(uint8_t len) {
    m_cmdRx.clear();
    QByteArray data;
    data.append(len);
    data.append(~len);
    dataSend(data);
    m_state = CmdSend;
    return true;
}

bool Flasher::writeData(const QByteArray &data) {
    m_cmdRx.clear();

    QByteArray data_send;
    data_send.append(data.length() - 1);
    data_send.append(data);

    uint8_t* check_data = (uint8_t*)(data_send.data());
    uint8_t check = check_data[0];
    for(int i = 1; i < data_send.length(); i++) {
        check ^= check_data[i];
    }

    data_send.append(check);
    dataSend(data_send);

    return true;
}
