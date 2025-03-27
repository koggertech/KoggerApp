#include "DeviceManagerWrapper.h"


DeviceManagerWrapper::DeviceManagerWrapper(QObject* parent) :
    QObject(parent),
    averageChartLosses_(0)
{
    workerObject_ = std::make_unique<DeviceManager>();

#ifdef SEPARATE_READING
    workerThread_ = std::make_unique<QThread>(this);
    auto connectionType = Qt::AutoConnection;
    deviceManagerConnections_.append(QObject::connect(this,                &DeviceManagerWrapper::sendOpenFile,  workerObject_.get(), &DeviceManager::openFile,             connectionType));
    deviceManagerConnections_.append(QObject::connect(this,                &DeviceManagerWrapper::sendCloseFile, workerObject_.get(), &DeviceManager::closeFile,            connectionType));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::devChanged,           this,                &DeviceManagerWrapper::devChanged,    connectionType));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::streamChanged,        this,                &DeviceManagerWrapper::streamChanged, connectionType));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::vruChanged,           this,                &DeviceManagerWrapper::vruChanged,    connectionType));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::chartLossesChanged,   this,                &DeviceManagerWrapper::calcAverageChartLosses,    connectionType));


#ifdef MOTOR
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::motorDeviceChanged,       this,                &DeviceManagerWrapper::motorDeviceChanged, connectionType));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::anglesHasChanged,         this,                &DeviceManagerWrapper::angleChanged,       connectionType));
    deviceManagerConnections_.append(QObject::connect(workerObject_.get(), &DeviceManager::posIsConstant,            this,                &DeviceManagerWrapper::posIsConstant,      connectionType));
    deviceManagerConnections_.append(QObject::connect(this,                &DeviceManagerWrapper::sendRunSteps,      workerObject_.get(), &DeviceManager::runSteps,                  connectionType));
    deviceManagerConnections_.append(QObject::connect(this,                &DeviceManagerWrapper::sendReturnToZero,  workerObject_.get(), &DeviceManager::returnToZero,              connectionType));
    deviceManagerConnections_.append(QObject::connect(this,                &DeviceManagerWrapper::sendOpenCsvFile,   workerObject_.get(), &DeviceManager::openCsvFile,               connectionType));
    deviceManagerConnections_.append(QObject::connect(this,                &DeviceManagerWrapper::sendClearTasks,    workerObject_.get(), &DeviceManager::clearTasks,                connectionType));
#endif

    workerObject_->moveToThread(workerThread_.get());
    workerThread_->start();
#else
    auto connectionType = Qt::DirectConnection;
    QObject::connect(this,                &DeviceManagerWrapper::sendOpenFile,  workerObject_.get(), &DeviceManager::openFile,             connectionType);
    QObject::connect(this,                &DeviceManagerWrapper::sendCloseFile, workerObject_.get(), &DeviceManager::closeFile,            connectionType);
    QObject::connect(workerObject_.get(), &DeviceManager::devChanged,           this,                &DeviceManagerWrapper::devChanged,    connectionType);
    QObject::connect(workerObject_.get(), &DeviceManager::streamChanged,        this,                &DeviceManagerWrapper::streamChanged, connectionType);
    QObject::connect(workerObject_.get(), &DeviceManager::vruChanged,           this,                &DeviceManagerWrapper::vruChanged,    connectionType);
    QObject::connect(workerObject_.get(), &DeviceManager::chartLossesChanged,   this,                &DeviceManagerWrapper::calcAverageChartLosses,    connectionType);


#ifdef MOTOR
    QObject::connect(workerObject_.get(), &DeviceManager::motorDeviceChanged,       this,                &DeviceManagerWrapper::motorDeviceChanged, connectionType);
    QObject::connect(workerObject_.get(), &DeviceManager::anglesHasChanged,         this,                &DeviceManagerWrapper::angleChanged,       connectionType);
    QObject::connect(workerObject_.get(), &DeviceManager::posIsConstant,            this,                &DeviceManagerWrapper::posIsConstant,      connectionType);
    QObject::connect(this,                &DeviceManagerWrapper::sendRunSteps,      workerObject_.get(), &DeviceManager::runSteps,                  connectionType);
    QObject::connect(this,                &DeviceManagerWrapper::sendReturnToZero,  workerObject_.get(), &DeviceManager::returnToZero,              connectionType);
    QObject::connect(this,                &DeviceManagerWrapper::sendOpenCsvFile,   workerObject_.get(), &DeviceManager::openCsvFile,               connectionType);
    QObject::connect(this,                &DeviceManagerWrapper::sendClearTasks,    workerObject_.get(), &DeviceManager::clearTasks,                connectionType);
#endif

#endif
}

DeviceManagerWrapper::~DeviceManagerWrapper()
{
#ifdef SEPARATE_READING
    for (auto& itm : deviceManagerConnections_)
        QObject::disconnect(itm);
    deviceManagerConnections_.clear();

    workerThread_->quit();
    workerThread_->wait();

    workerThread_.reset();
    workerObject_.reset();
#endif
}

DeviceManager* DeviceManagerWrapper::getWorker()
{
    return workerObject_.get();
}

void DeviceManagerWrapper::calcAverageChartLosses()
{
    averageChartLosses_ = std::max(0, std::min(100, 100 - getWorker()->calcAverageChartLosses()));
    emit this->chartLossesChanged();
}

#ifdef MOTOR
void DeviceManagerWrapper::posIsConstant(float currFAngle, float taskFAngle, float currSAngle, float taskSAngle)
{
    qDebug() << "DeviceManagerWrapper::posIsConstant: currFAngle: " << currFAngle << ", taskFAngle: " << taskFAngle << ", currSAngle: " << currSAngle << ", taskSAngle: " << taskSAngle;

    currFAngle_ = currFAngle;
    currSAngle_ = currSAngle;
    taskFAngle_ = taskFAngle;
    taskSAngle_ = taskSAngle;

    emit enginesStopped();
}
#endif
