#include "DeviceManagerWrapper.h"


DeviceManagerWrapper::DeviceManagerWrapper(QObject* parent) :
    QObject(parent)
{
    //workerThread_ = std::make_unique<QThread>(this);
    workerObject_ = std::make_unique<DeviceManager>();

    // workerObject_->moveToThread(workerThread_.get());

    // connects

    auto connectionType = Qt::DirectConnection; // one thread
    QObject::connect(this,                &DeviceManagerWrapper::sendOpenFile,  workerObject_.get(), &DeviceManager::openFile,              connectionType);
    QObject::connect(this,                &DeviceManagerWrapper::sendCloseFile, workerObject_.get(), &DeviceManager::closeFile,             connectionType);
    QObject::connect(workerObject_.get(), &DeviceManager::devChanged,           this,                &DeviceManagerWrapper::devChanged,     connectionType);
    QObject::connect(workerObject_.get(), &DeviceManager::streamChanged,        this,                &DeviceManagerWrapper::streamChanged,  connectionType);
    QObject::connect(workerObject_.get(), &DeviceManager::vruChanged,           this,                &DeviceManagerWrapper::vruChanged,     connectionType);

    QObject::connect(workerObject_.get(), &DeviceManager::motorDeviceChanged,       this,                &DeviceManagerWrapper::motorDeviceChanged, connectionType);
    QObject::connect(workerObject_.get(), &DeviceManager::anglesHasChanged,         this,                &DeviceManagerWrapper::angleChanged,       connectionType);
    QObject::connect(workerObject_.get(), &DeviceManager::calibrationStandIn,            this,                &DeviceManagerWrapper::posIsConstant,      connectionType);
    QObject::connect(this,                &DeviceManagerWrapper::sendRunSteps,      workerObject_.get(), &DeviceManager::runSteps,                  connectionType);
    QObject::connect(this,                &DeviceManagerWrapper::sendReturnToZero,  workerObject_.get(), &DeviceManager::returnToZero,              connectionType);
    QObject::connect(this,                &DeviceManagerWrapper::sendOpenCsvFile,   workerObject_.get(), &DeviceManager::openCsvFile,               connectionType);
    QObject::connect(this,                &DeviceManagerWrapper::sendClearTasks,    workerObject_.get(), &DeviceManager::clearTasks,                connectionType);

    //workerThread_->start();
}

DeviceManagerWrapper::~DeviceManagerWrapper()
{
    // if (workerThread_ && workerThread_->isRunning()) {
    //     workerThread_->quit();
    //     workerThread_->wait();
    // }

    // workerThread_->deleteLater();
}

DeviceManager* DeviceManagerWrapper::getWorker()
{
    return workerObject_.get();
}

void DeviceManagerWrapper::posIsConstant(float currFAngle, float taskFAngle, float currSAngle, float taskSAngle)
{
    qDebug() << "DeviceManagerWrapper::posIsConstant: currFAngle: " << currFAngle << ", taskFAngle: " << taskFAngle << ", currSAngle: " << currSAngle << ", taskSAngle: " << taskSAngle;

    currFAngle_ = currFAngle;
    currSAngle_ = currSAngle;
    taskFAngle_ = taskFAngle;
    taskSAngle_ = taskSAngle;

    emit enginesStopped();
}
