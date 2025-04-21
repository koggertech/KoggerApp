#pragma once

#include <QObject>
#include <QVector>
#include <QVector3D>


class IsobathsProcessorTask
{
public:
    QVector<QVector3D> grid;
    int gridWidth = 0;
    int gridHeight = 0;
    float step = 1.f;

private:
    friend class IsobathsProcessor;
};
Q_DECLARE_METATYPE(IsobathsProcessorTask)

class IsobathsProcessor : public QObject
{
    Q_OBJECT

public:
    struct IsobathProcessorResult {
        QVector <QVector3D> data;
    };

    explicit IsobathsProcessor(QObject *parent = nullptr);
    virtual ~IsobathsProcessor();

    bool setTask(const IsobathsProcessorTask& task);
    bool startInThread();
    bool startInThread(const IsobathsProcessorTask& task);
    bool stopInThread(unsigned long time = ULONG_MAX);
    IsobathsProcessorTask task() const;
    const IsobathsProcessorTask& ctask() const;

    bool isBusy() const;
    IsobathProcessorResult result() const;

private Q_SLOTS:
    void process();

Q_SIGNALS:
    void taskStarted();
    void taskFinished(IsobathsProcessor::IsobathProcessorResult result);

private:
    QVector<QVector3D> buildGridTriangles(const QVector<QVector3D>& pts, int gridWidth, int gridHeight);

    IsobathsProcessorTask task_;
    IsobathProcessorResult result_;
    std::atomic_bool isBusy_;
};
