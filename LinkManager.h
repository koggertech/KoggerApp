#pragma once

#include <QHash>
#include <QThread>
#include <QUuid>

#include "Link.h"
#include "LinkListModel.h"
#include "LinkManagerWorker.h"


class LinkManager : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(LinkListModel* linkListModel READ getModelPtr NOTIFY stateChanged)

    /*methods*/
    LinkManager();
    QHash<QUuid, Link> getHash() const;
    LinkListModel* getModelPtr();

private:
    /*methods*/
    Link* getLinkPtr(QUuid uuid);
    /*data*/
    std::unique_ptr<QThread> workerThread_;
    std::unique_ptr<LinkManagerWorker> workerObject_;
    QHash<QUuid, Link> hash_;
    LinkListModel model_;

signals:
    void stateChanged();

public slots:
    void open(QUuid uuid);
    void close(QUuid uuid);
};

