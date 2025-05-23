#pragma once

#include <QObject>
#include <QVector>
#include <QVector3D>


using IsobathsPolylines = QVector<QVector<QVector3D>>;
using IsobathsSegVec = QVector<QPair<QVector3D,QVector3D>>;

struct LabelInfo
{
    QVector3D pos;
    QVector3D dir;
    float depth;
};

struct IsobathsProcessorResult {
    QVector<QVector3D> data;
    QVector<LabelInfo> labels;
};

class IsobathsProcessorTask
{
public:
    QVector<QVector3D> grid;
    int gridWidth = 0;
    int gridHeight = 0;
    float step = 1.0f;
    float labelStep  = 100.0f;

private:
    friend class IsobathsProcessor;
};
Q_DECLARE_METATYPE(IsobathsProcessorTask)

class IsobathsProcessor : public QObject
{
    Q_OBJECT

public:
    explicit IsobathsProcessor(QObject *parent = nullptr);
    virtual ~IsobathsProcessor();

    bool setTask(const IsobathsProcessorTask& task);
    bool startInThread();
    bool startInThread(const IsobathsProcessorTask& task);
    bool stopInThread(unsigned long time = ULONG_MAX);
    IsobathsProcessorTask task() const;
    const IsobathsProcessorTask& ctask() const;

    bool isBusy() const;
    IsobathsProcessorResult result() const;

private Q_SLOTS:
    void process();

Q_SIGNALS:
    void taskStarted();
    void taskFinished(IsobathsProcessorResult result);

private:
    QVector<QVector3D> buildGridTriangles(const QVector<QVector3D>& pts, int gridWidth, int gridHeight) const;
    void buildPolylines(const IsobathsSegVec& segs, IsobathsPolylines& polylines) const;
    void edgeIntersection(const QVector3D& vertA, const QVector3D& vertB, float level, QVector<QVector3D>& out) const;
    void filterNearbyLabels(const QVector<LabelInfo>& inputData, QVector<LabelInfo>& outputData) const;
    void filterLinesBehindLabels(const QVector<LabelInfo>& filteredLabels, const QVector<QVector3D>& inputData, QVector<QVector3D>& outputData) const;

    IsobathsProcessorTask task_;
    IsobathsProcessorResult result_;
    std::atomic_bool isBusy_;
};
