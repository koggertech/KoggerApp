#pragma once

#include <QHash>
#include <QList>
#include <QPair>
#include <QSerialPortInfo>
#include <QUuid>

#include "Link.h"


namespace linking {

class LinkManager : public QObject {
    Q_OBJECT
public:
    /*methods*/
    explicit LinkManager();
    ~LinkManager();
    QPair<QUuid, Link> createSerialPort(const QSerialPortInfo& serialInfo) const;
    QHash<QUuid, Link> createSerialPortsByDefault();

private:
    /*methods*/
    QList<QSerialPortInfo> getSerialList() const;
    void updateLinksList();
    void addNewLinks(const QList<QSerialPortInfo> &currSerialList);
    void deleteMissingLinks(const QList<QSerialPortInfo> &currSerialList);

    /*data*/
    QHash<QUuid,Link> linkHash_;

public slots:

private slots:

signals:
};

}
