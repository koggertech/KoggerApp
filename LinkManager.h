#pragma once

#include <QHash>
#include <QList>
#include <QPair>
#include <QSerialPortInfo>
#include <QUuid>

#include "Link.h"
#include "LinkListModel.h"


class LinkManager : public QObject {
    Q_OBJECT
public:

    Q_PROPERTY(LinkListModel* linkListModel READ getLinkModel NOTIFY linkModelChanged)

    /*methods*/
    explicit LinkManager();
    ~LinkManager();
    QHash<QUuid, Link> createSerialPortsByDefault();
    QHash<QUuid, Link> getLinkHash();

    LinkListModel* getLinkModel();
    void updateLinkModel(const QString& portName);

private:
    /*methods*/
    QPair<QUuid, Link> createSerialPort(const QSerialPortInfo& serialInfo) const;
    QList<QSerialPortInfo> getSerialList() const;
    void updateLinksList();
    bool addNewLinks(const QList<QSerialPortInfo> &currSerialList);
    bool deleteMissingLinks(const QList<QSerialPortInfo> &currSerialList);

    /*data*/
    QHash<QUuid, Link> linkHash_;
    LinkListModel linkModel_;

public slots:
    void addLink();

private slots:

signals:
    void linkHashChanged();
    void linkModelChanged();
};

