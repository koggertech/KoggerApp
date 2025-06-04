#include "surface_view_processor.h"

#include <QThread>
#include <QDebug>
#include <QVector2D>
#include <cmath>


static const float epsilon_ = 1e-6f;

static const QString& underlyingThreadName()
{
    static const QString name = QStringLiteral("SurfaceViewProcessorThread");
    return name;
}

SurfaceViewProcessor::SurfaceViewProcessor(QObject *parent) :
    QObject(parent),
    isBusy_(false)
{
    qRegisterMetaType<SurfaceViewProcessorResult>("SurfaceViewProcessorResult");
}

SurfaceViewProcessor::~SurfaceViewProcessor()
{
    stopInThread();
}

bool SurfaceViewProcessor::setTask(const SurfaceViewProcessorTask& task)
{
    if (isBusy_.load()) {
        return false;
    }

    task_ = task;

    return true;
}

bool SurfaceViewProcessor::startInThread()
{
    if (isBusy_.load()) {
        qDebug() << "SurfaceViewProcessor::startInThread: isBusy_.load() failed";
        return false;
    }

    if (parent()) {
        qDebug() << "SurfaceViewProcessor::startInThread: parent() failed";

        return false;
    }

    auto currentThread = thread();

    if ((currentThread && currentThread->objectName() != underlyingThreadName()) || !currentThread) {
        currentThread = new QThread(this);
        currentThread->setObjectName(underlyingThreadName());
        QObject::connect(currentThread, &QThread::started, this, &SurfaceViewProcessor::process);
        moveToThread(currentThread);
    }

    currentThread->start();
    return true;
}

bool SurfaceViewProcessor::startInThread(const SurfaceViewProcessorTask &task)
{
    if (!setTask(task)) {
        return false;
    }

    return startInThread();
}

bool SurfaceViewProcessor::stopInThread(unsigned long time)
{
    auto* currentThread = thread();

    if (parent() || !(currentThread && currentThread->objectName() == underlyingThreadName())) {
        return true;
    }

    if (QThread::currentThread() == currentThread) {
        currentThread->quit();
        return true;
    }

    currentThread->quit();

    return currentThread->wait(time);
}

SurfaceViewProcessorTask SurfaceViewProcessor::task() const
{
    return task_;
}

const SurfaceViewProcessorTask &SurfaceViewProcessor::ctask() const
{
    return task_;
}

bool SurfaceViewProcessor::isBusy() const
{
    return isBusy_.load();
}

SurfaceViewProcessorResult SurfaceViewProcessor::result() const
{
    return result_;
}

void SurfaceViewProcessor::process()
{
    if (isBusy_.exchange(true)) {
        return;
    }

    emit taskStarted();
    result_.data.clear();

    const auto& data = task_.grid;
    const int dataSize = data.size();

    if (dataSize < 3 || task_.step <= 0.f) {
        isBusy_ = false;
        emit taskFinished(result_);
        stopInThread();
        return;
    }



    // processing



    isBusy_ = false;
    emit taskFinished(result_);
    stopInThread();
}
