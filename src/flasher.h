#ifndef FLASHER_H
#define FLASHER_H

#include <QObject>

class Flasher : public QObject
{
    Q_OBJECT
public:
    explicit Flasher(QObject *parent = nullptr);

    Q_PROPERTY(int writeProgress READ getWriteProgress NOTIFY writeProgressChanged)
    Q_PROPERTY(int readProgress READ getReadProgress NOTIFY readProgressChanged)
    Q_PROPERTY(int checkProgress READ getCheckProgress NOTIFY checkProgressChanged)

    typedef enum {
        BootNone,
        BootAsk,
        BootLock,
        BootUnLock,
        BootUnLockFail,
        BootReady,
        BootReadSuccess,
        BootWriteSuccess,
        BootErase,
    } BootState;

signals:
    void dataSend(QByteArray data);
    void writeProgressChanged();
    void readProgressChanged();
    void checkProgressChanged();
    void connectionChanged(BootState connection_status);


public slots:
    void putData(const QByteArray &data);
    void getVerAndRDP();
    void read(uint32_t addr, uint32_t size);
    void write(uint32_t addr, const QByteArray &data);
    void unprotect();
    void erase();
    QByteArray getRead() { return m_readBuf; }
    bool check(QByteArray &data);

    int getWriteProgress() { return m_writeProgress; }
    int getReadProgress() { return m_readProgress; }
    int getCheckProgress() { return m_checkProgress; }
    void startConnection(bool duplex);

protected:
    enum {
        Idle,
        CmdSend,
        RxAsk,
        RxNAsk,
        Undef
    } m_state;

    enum {
        readIdle,
        readCmdAsk,
        readAddrAsk,
    } m_readState;

    enum {
        writeIdle,
        writeCmdAsk,
        writeDataAsk,
    } m_writeState;

    enum {
        ACK = 0x79,
        NACK = 0x1F
    };

    typedef enum {
        CMD_GET = 0x00,
        CMD_VER_RDP = 0x01,
        CMD_ChipID = 0x02,
        CMD_ReadMemory = 0x11,
        CMD_Go = 0x21,
        CMD_WriteMemory = 0x31,
        CMD_Erase = 0x43,
        CMD_ExtErase = 0x44,
        CMD_Unlock = 0x92,
        CMD_Connect = 0xFE,
        CMD_NONE = 0xFF
    } CMD_ID;

    QByteArray m_cmdRx;
    CMD_ID lastCMD = CMD_NONE;

    uint8_t bootVer = 0;
    uint8_t OB[2] = {};

    bool m_connect = false;
    bool m_testRDP = false;
    uint32_t unlockCnt = 0;

    QByteArray m_readBuf;
    uint32_t m_readAddr = 0x08000000;
    uint32_t m_readLastLen = 0;
    uint32_t m_readAddrEnd = 0x08000000 + 1*1024;
    uint32_t m_readMax = 0;
    uint32_t m_readProgress = 0;

    QByteArray m_writeBuf;
    uint32_t m_writeAddr = 0x08000000;
    uint32_t m_writeLen = 0;
    uint32_t m_writeMax = 0;
    uint32_t m_writeProgress = 0;

    uint32_t m_checkProgress = 0;

    bool sendCMD(CMD_ID id);
    bool sendAddr(uint32_t addr);
    bool sendLen(uint8_t len);
    bool writeData(const QByteArray &data);

};

#endif // FLASHER_H
