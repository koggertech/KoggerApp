#include "control_server.h"

#include "control_listener.h"
#include "command_dispatcher.h"

#include <QThread>
#include <QMetaObject>
#include <QDebug>

ControlServer::ControlServer(QObject* parent)
    : QObject(parent),
      dispatcher_(new CommandDispatcher(this))
{
}

ControlServer::~ControlServer()
{
    stop();
}

void ControlServer::registerObject(const QString& name, QObject* object)
{
    dispatcher_->registerObject(name, object);
}

void ControlServer::setUiProbe(UiProbe* probe)
{
    dispatcher_->setUiProbe(probe);
}

void ControlServer::setArtifactRoot(const QString& dir)
{
    dispatcher_->setArtifactRoot(dir);
}

bool ControlServer::start(const QHostAddress& address, quint16 port, const QString& token)
{
    if (thread_) {
        return false;
    }

    thread_ = std::make_unique<QThread>(this);
    thread_->setObjectName(QStringLiteral("ControlThread"));

    listener_ = new ControlListener();
    listener_->moveToThread(thread_.get());
    connect(thread_.get(), &QThread::finished, listener_, &QObject::deleteLater);

    connect(listener_, &ControlListener::commandReceived, dispatcher_, &CommandDispatcher::dispatch,   Qt::QueuedConnection);
    connect(dispatcher_, &CommandDispatcher::taken,       listener_,   &ControlListener::onTaken,      Qt::QueuedConnection);
    connect(dispatcher_, &CommandDispatcher::reply,       listener_,   &ControlListener::sendReply,    Qt::QueuedConnection);

    connect(listener_, &ControlListener::listening, this, [](const QHostAddress& addr, quint16 p) {
        qInfo().noquote() << QStringLiteral("control: listening %1:%2").arg(addr.toString()).arg(p);
    }, Qt::QueuedConnection);
    connect(listener_, &ControlListener::listenFailed, this, [](const QString& error) {
        qWarning().noquote() << QStringLiteral("control: %1").arg(error);
    }, Qt::QueuedConnection);

    thread_->start();
    QMetaObject::invokeMethod(listener_, [this, address, port, token]() {
        listener_->listen(address, port, token);
    }, Qt::QueuedConnection);
    return true;
}

void ControlServer::stop()
{
    if (!thread_) {
        return;
    }
    if (listener_ && thread_->isRunning()) {
        QMetaObject::invokeMethod(listener_, &ControlListener::close, Qt::BlockingQueuedConnection);
    }
    thread_->quit();
    thread_->wait();
    listener_ = nullptr;
    thread_.reset();
}
