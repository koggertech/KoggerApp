#pragma once

#include <QHash>
#include <QList>
#include <QPair>
#include <QSerialPortInfo>
#include <QTimer>
#include <QUuid>

#include "Link.h"
#include "LinkListModel.h"


class LinkManager : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(LinkListModel* linkListModel READ getModelPtr NOTIFY stateChanged)

    /*methods*/
    LinkManager();
    ~LinkManager();

    void update(); // TODO: starts by timer

    QHash<QUuid, Link> getHash() const;
    LinkListModel* getModelPtr();

private:
    /*methods*/
    QList<QSerialPortInfo> getSerialList() const;
    QPair<QUuid, Link> createSerialPort(const QSerialPortInfo& serialInfo) const;

    void addNewLinks(const QList<QSerialPortInfo> &currSerialList);
    void deleteMissingLinks(const QList<QSerialPortInfo> &currSerialList);

    /*data*/
    QHash<QUuid, Link> hash_;
    LinkListModel model_;

    QTimer timer_;
    static const int timerInterval_ = 200; // msecs

public slots:

private slots:
    void onExpiredTimer();

signals:
    void stateChanged();
};

