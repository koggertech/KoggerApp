#include "surface_processor.h"

#include <set>
#include <memory>

#include "point_3d.h"
#include "delaunay_triangulation.h"
#include "grid_generator.h"
#include "barycentric_interpolator.h"
#include "bottom_track.h"

const QString UnderlyingThreadName = "SurfaceProcessorThread";

SurfaceProcessor::SurfaceProcessor(QObject *parent)
    : QObject{parent}
{
    qRegisterMetaType<Result>("Result");
}

SurfaceProcessor::~SurfaceProcessor()
{
    stopInThread();
}

bool SurfaceProcessor::setTask(const SurfaceProcessorTask& task)
{
    if(m_isBusy.load())
        return false;

    m_task = task;

    return true;
}

bool SurfaceProcessor::startInThread()
{
    if(m_isBusy.load())
        return false;

    if (parent())
        return false;

    auto currentThread = thread();

    if((currentThread && currentThread->objectName() != UnderlyingThreadName) || !currentThread){
        currentThread = new QThread(this);
        currentThread->setObjectName(UnderlyingThreadName);
        QObject::connect(currentThread, &QThread::started, this, &SurfaceProcessor::process);
        moveToThread(currentThread);
    }

    currentThread->start();
    return true;
}

bool SurfaceProcessor::startInThread(const SurfaceProcessorTask &task)
{
    if(!setTask(task))
        return false;

    return startInThread();
}

bool SurfaceProcessor::stopInThread(unsigned long time)
{
    auto currentThread = thread();

    if(parent() || !(currentThread && currentThread->objectName() == UnderlyingThreadName))
        return true;

    if (QThread::currentThread() == currentThread) { // same thread
        currentThread->quit();
        return true;
    }

    currentThread->quit();

    return currentThread->wait(time);
}

SurfaceProcessorTask SurfaceProcessor::task() const
{
    return m_task;
}

const SurfaceProcessorTask &SurfaceProcessor::ctask() const
{
    return m_task;
}

void SurfaceProcessor::process()
{
    if(m_isBusy.load())
        return;

    if(m_task.bottomTrack()->cdata().isEmpty()){
        stopInThread();
        return;
    }

    m_isBusy.store(true);

    m_result.data.clear();
    m_result.primitiveType = GL_TRIANGLES;

    Q_EMIT taskStarted();

    std::set <Point2D <double>> set;
    std::vector <Point3D <double>> input;


    QVector<QVector3D> data = m_task.bottomTrack()->data();

    if(m_task.m_bottomTrackDataFilter){
        QVector<QVector3D> filtered;
        m_task.m_bottomTrackDataFilter->apply(data, filtered);
        data = filtered;
    }

    for (int i = 0; i < data.size(); i++){
        Point2D <double> point(static_cast <double> (data.at(i).x()),
                               static_cast <double> (data.at(i).y()),
                               static_cast <double> (i));

        set.insert(point);
    }

    for (const auto& p : set){
        Point3D <double> point(p.x(),
                               p.y(),
                               static_cast <double> (data.at(p.index()).z()),
                               p.index());
        input.push_back(point);
    }

    Delaunay <double> delaunay;
    auto triangles = delaunay.trinagulate(input, m_task.m_edgeLengthLimit);

    if (m_task.m_gridInterpEnabled) {
        m_result.primitiveType = GL_QUADS;

        std::vector <Point3D <double>> trimmedGrid;

        Cube bounds = m_task.m_bottomTrack.lock()->bounds();
        auto fullGrid = GridGenerator <double>::generateQuadGrid(Point3D <double>(
                                                                bounds.minimumX(),
                                                                bounds.minimumY(),
                                                                bounds.minimumZ()
                                                            ),
                                                            bounds.width(),
                                                            bounds.length(),
                                                            m_task.m_interpGridCellSize);

        auto q = fullGrid->begin();

        while (q != fullGrid->end()){
            auto quad = *q;

            bool surfaceContainsA = false;
            bool surfaceContainsB = false;
            bool surfaceContainsC = false;
            bool surfaceContainsD = false;

            auto t = triangles->begin();
            while (t != triangles->end() && (!surfaceContainsA ||
                                             !surfaceContainsB ||
                                             !surfaceContainsC ||
                                             !surfaceContainsD))
            {
                if (!surfaceContainsA) surfaceContainsA = t->contains(quad.A());
                if (!surfaceContainsB) surfaceContainsB = t->contains(quad.B());
                if (!surfaceContainsC) surfaceContainsC = t->contains(quad.C());
                if (!surfaceContainsD) surfaceContainsD = t->contains(quad.D());
                t++;
            }

            if (t != triangles->end()){
                trimmedGrid.push_back(quad.A());
                trimmedGrid.push_back(quad.B());
                trimmedGrid.push_back(quad.C());
                trimmedGrid.push_back(quad.D());
            }
            q++;
        }

        BarycentricInterpolator <double> interpolator;

        auto trianglesTemp = *triangles;

        interpolator.process(trianglesTemp, trimmedGrid);

        for(const auto& point : trimmedGrid)
            m_result.data.append(point.toQVector3D());

    }
    else {
        for (const auto& t : *triangles) {
            m_result.data.append(t.A().toQVector3D());
            m_result.data.append(t.B().toQVector3D());
            m_result.data.append(t.C().toQVector3D());
        }
    }

    m_isBusy.store(false);

    Q_EMIT taskFinished(m_result);

    stopInThread();
}

bool SurfaceProcessor::isBusy() const
{
    return m_isBusy.load();
}

void SurfaceProcessorTask::setBottomTrack(std::weak_ptr<BottomTrack> bottomTrack)
{
    if(m_bottomTrack.lock() != bottomTrack.lock())
        m_bottomTrack = bottomTrack;
}

void SurfaceProcessorTask::setGridInterpEnabled(bool enabled)
{
    if(m_gridInterpEnabled != enabled)
        m_gridInterpEnabled = enabled;
}

void SurfaceProcessorTask::setInterpGridCellSize(qreal size)
{
    if(m_interpGridCellSize != size)
        m_interpGridCellSize = size;
}

void SurfaceProcessorTask::setEdgeLengthLimit(qreal limit)
{
    if(m_edgeLengthLimit != limit)
        m_edgeLengthLimit = limit;
}

void SurfaceProcessorTask::setBottomTrackDataFilter(std::shared_ptr<AbstractEntityDataFilter> filter)
{
    if(m_bottomTrackDataFilter != filter)
        m_bottomTrackDataFilter = filter;
}

BottomTrack *SurfaceProcessorTask::bottomTrack() const
{
    return m_bottomTrack.lock().get();
}

bool SurfaceProcessorTask::gridInterpEnabled() const
{
    return m_gridInterpEnabled;
}

qreal SurfaceProcessorTask::interpGridCellSize() const
{
    return m_interpGridCellSize;
}

qreal SurfaceProcessorTask::edgeLengthLimit() const
{
    return m_edgeLengthLimit;
}

AbstractEntityDataFilter *SurfaceProcessorTask::bottomTrackDataFilter() const
{
    return m_bottomTrackDataFilter.get();
}
