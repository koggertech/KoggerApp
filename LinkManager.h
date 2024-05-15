#pragma once

#include <QHash>
#include <QList>
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

    QList<Link*> getLinkPtrList() const;
    LinkListModel* getModelPtr();

private:
    /*methods*/
    Link* getLinkPtr(QUuid uuid);
    /*data*/
    std::unique_ptr<QThread> workerThread_;
    std::unique_ptr<LinkManagerWorker> workerObject_;

    QList<Link*> list_;
    LinkListModel model_;

signals:
    void stateChanged();
    void openedEvent(bool);

public slots:
    void open(QUuid uuid);
    void close(QUuid uuid);
};

