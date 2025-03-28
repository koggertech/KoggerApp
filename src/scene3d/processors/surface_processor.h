#ifndef SURFACEPROCESSOR_H
#define SURFACEPROCESSOR_H

#include <qsystemdetection.h>
#if !defined(Q_OS_ANDROID) && !defined(LINUX_ES)
#include <GL/gl.h>
#else
#include <GLES2/gl2.h>
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>
#endif

#include <memory>
#include <atomic>

#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QThread>

#include "cube.h"
#include "abstract_entity_data_filter.h"

class BottomTrack;
class SurfaceProcessorTask{
    Q_GADGET
    Q_PROPERTY(BottomTrack*              bottomTrack           READ bottomTrack           CONSTANT)
    Q_PROPERTY(bool                      gridInterpEnabled     READ gridInterpEnabled     CONSTANT)
    Q_PROPERTY(qreal                     interpGridCellSize    READ interpGridCellSize    CONSTANT)
    Q_PROPERTY(qreal                     edgeLengthLimit       READ edgeLengthLimit       CONSTANT)
    Q_PROPERTY(AbstractEntityDataFilter* bottomTrackDataFilter READ bottomTrackDataFilter CONSTANT)

public:
    void setBottomTrack(std::weak_ptr<BottomTrack> bottomTrack);
    void setGridInterpEnabled(bool enabled);
    void setInterpGridCellSize(qreal size);
    void setEdgeLengthLimit(qreal limit);
    void setBottomTrackDataFilter(std::shared_ptr<AbstractEntityDataFilter> filter);
    BottomTrack* bottomTrack() const;
    bool gridInterpEnabled() const;
    qreal interpGridCellSize() const;
    qreal edgeLengthLimit() const;
    AbstractEntityDataFilter *bottomTrackDataFilter() const;

private:
    friend class SurfaceProcessor;
    bool m_gridInterpEnabled = false;
    qreal m_interpGridCellSize = 5.0f;
    qreal m_edgeLengthLimit = -1.0f;
    std::shared_ptr<AbstractEntityDataFilter> m_bottomTrackDataFilter;
    std::weak_ptr<BottomTrack> m_bottomTrack;
};
Q_DECLARE_METATYPE(SurfaceProcessorTask)

class SurfaceProcessor : public QObject
{
    Q_OBJECT

public:
    struct Result{
        QVector <QVector3D> data;
        int primitiveType = GL_TRIANGLES;
    };

    explicit SurfaceProcessor(QObject *parent = nullptr);
    virtual ~SurfaceProcessor();

    bool setTask(const SurfaceProcessorTask& task);
    bool startInThread();
    bool startInThread(const SurfaceProcessorTask& task);
    bool stopInThread(unsigned long time = ULONG_MAX);
    SurfaceProcessorTask task() const;
    const SurfaceProcessorTask& ctask() const;

    bool isBusy() const;
    Result result() const;

private Q_SLOTS:
    void process();

Q_SIGNALS:
    void taskStarted();
    void taskFinished(Result result);

private:
    SurfaceProcessorTask m_task;
    Result m_result;
    std::atomic_bool m_isBusy{false};
};

#endif // SURFACEPROCESSOR_H
