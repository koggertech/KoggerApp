#pragma once

#include <memory>

#include <QObject>
#include <QTimer>
#include <QList>
#include <QUuid>
#include <QSerialPortInfo>

#include "Link.h"


class LinkManager : public QObject
{
    Q_OBJECT
public:
    /*methods*/
    explicit LinkManager(QObject *parent = nullptr);
    ~LinkManager();

private:
    /*methods*/
    QList<QSerialPortInfo> getCurrentSerialList() const;
    Link* createSerialPort(const QSerialPortInfo& serialInfo) const;
    void addNewLinks(const QList<QSerialPortInfo> &currSerialList);
    void deleteMissingLinks(const QList<QSerialPortInfo> &currSerialList);
    void update();
    Link *getLinkPtr(QUuid uuid);

    /*data*/
    QList<Link*> list_;
    std::unique_ptr<QTimer> timer_;
    static const int timerInterval_ = 500; // msecs
    QMutex mutex_;

signals:
    void appendModifyModel(QUuid uuid, bool connectionStatus, ControlType controlType, QString portName, int baudrate, bool parity,
                       LinkType linkType, QString address, int sourcePort, int destinationPort, bool isPinned, bool isHided, bool isNotAvailable);
    void deleteModel(QUuid uuid);
    void frameReady(Link* link, FrameParser frame); //

public slots:
    void onLinkConnectionStatusChanged(QUuid uuid);

    void onExpiredTimer();

    void openAsSerial(QUuid uuid);
    void openAsUdp(QUuid uuid);
    void openAsTcp(QUuid uuid);

    void close(QUuid uuid);

    void frameInput(Link* link, FrameParser frame); //
};
