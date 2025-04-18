#include "isobaths_processor.h"

#include <QThread>
#include <QDebug>

static const QString& underlyingThreadName()
{
    static const QString name = QStringLiteral("IsobathsProcessorThread");
    return name;
}


IsobathsProcessor::IsobathsProcessor(QObject *parent) :
    QObject(parent),
    isBusy_(false)
{
    qDebug() << "   ISOBATHS_PROCESSOR: ctr in thread" << QThread::currentThreadId();
    qRegisterMetaType<IsobathsProcessor::IsobathProcessorResult>("IsobathsProcessor::IsobathProcessorResult");
}

IsobathsProcessor::~IsobathsProcessor()
{
    stopInThread();
}

bool IsobathsProcessor::setTask(const IsobathsProcessorTask& task)
{
    if (isBusy_.load()) {
        return false;
    }

    task_ = task;

    return true;
}

bool IsobathsProcessor::startInThread()
{
    if (isBusy_.load()) {
        return false;
    }

    if (parent()) {
        return false;
    }

    auto currentThread = thread();

    if ((currentThread && currentThread->objectName() != underlyingThreadName()) || !currentThread) {
        currentThread = new QThread(this);
        currentThread->setObjectName(underlyingThreadName());
        QObject::connect(currentThread, &QThread::started, this, &IsobathsProcessor::process);
        moveToThread(currentThread);
    }

    currentThread->start();
    return true;
}

bool IsobathsProcessor::startInThread(const IsobathsProcessorTask &task)
{
    if (!setTask(task)) {
        return false;
    }

    return startInThread();
}

bool IsobathsProcessor::stopInThread(unsigned long time)
{
    auto currentThread = thread();

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

IsobathsProcessorTask IsobathsProcessor::task() const
{
    return task_;
}

const IsobathsProcessorTask &IsobathsProcessor::ctask() const
{
    return task_;
}

void IsobathsProcessor::process()
{
    if(isBusy_.load()) {
        return;
    }

    isBusy_.store(true);

    result_.data.clear();

    Q_EMIT taskStarted();

    ///////////////////////////
    // task
    qDebug() << "   ISOBATHS_PROCESSOR: processing isobats task in thread" << QThread::currentThreadId();
    ///////////////////////////

    isBusy_.store(false);

    Q_EMIT taskFinished(result_);

    stopInThread();
}

bool IsobathsProcessor::isBusy() const
{
    return isBusy_.load();
}

IsobathsProcessor::IsobathProcessorResult IsobathsProcessor::result() const
{
    return result_;
}
