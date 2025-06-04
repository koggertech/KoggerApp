#pragma once

#include <QObject>
#include <QVector>
#include <QVector3D>



struct SurfaceViewProcessorResult {
    QVector<QVector3D> data;
};

class SurfaceViewProcessorTask
{
public:
    QVector<QVector3D> grid;
    int gridWidth = 0;
    int gridHeight = 0;
    float step = 1.0f;
    float labelStep  = 100.0f;

private:
    friend class SurfaceViewProcessor;
};
Q_DECLARE_METATYPE(SurfaceViewProcessorTask)

class SurfaceViewProcessor : public QObject
{
    Q_OBJECT

public:
    explicit SurfaceViewProcessor(QObject *parent = nullptr);
    virtual ~SurfaceViewProcessor();

    bool setTask(const SurfaceViewProcessorTask& task);
    bool startInThread();
    bool startInThread(const SurfaceViewProcessorTask& task);
    bool stopInThread(unsigned long time = ULONG_MAX);
    SurfaceViewProcessorTask task() const;
    const SurfaceViewProcessorTask& ctask() const;

    bool isBusy() const;
    SurfaceViewProcessorResult result() const;

private Q_SLOTS:
    void process();

Q_SIGNALS:
    void taskStarted();
    void taskFinished(SurfaceViewProcessorResult result);

private:    
    SurfaceViewProcessorTask task_;
    SurfaceViewProcessorResult result_;
    std::atomic_bool isBusy_;
};
