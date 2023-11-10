#include "surfaceprocessor.h"

#include <set>

#include <Point3D.h>
#include <DelaunayTriangulation.h>
#include <gridgenerator.h>
#include <barycentricinterpolator.h>

const QString UnderlyingThreadName = "SurfaceProcessorThread";

SurfaceProcessor::SurfaceProcessor(QObject *parent)
    : QObject{parent}
{}

SurfaceProcessor::~SurfaceProcessor()
{
    stopInThread();
}

bool SurfaceProcessor::setTask(const Task& task)
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

bool SurfaceProcessor::startInThread(const Task &task)
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

    currentThread->quit();

    return currentThread->wait(time);
}

void SurfaceProcessor::process()
{
    if(m_isBusy.load())
        return;

    if(m_task.source.isEmpty())
        return;

    m_isBusy.store(true);

    m_result.data.clear();
    m_result.primitiveType = GL_TRIANGLES;

    Q_EMIT taskStarted();

    std::set <Point2D <double>> set;
    std::vector <Point3D <double>> input;

    for (int i = 0; i < m_task.source.size(); i++){
        Point2D <double> point(static_cast <double> (m_task.source.at(i).x()),
                               static_cast <double> (m_task.source.at(i).y()),
                               static_cast <double> (i));

        set.insert(point);
    }

    for (const auto& p : set){
        Point3D <double> point(p.x(),
                               p.y(),
                               static_cast <double> (m_task.source.at(p.index()).z()),
                               p.index());
        input.push_back(point);
    }

    Delaunay <double> delaunay;
    auto triangles = delaunay.trinagulate(input, m_task.edgeLengthLimit);

    if(m_task.needSmoothing){
        m_result.primitiveType = GL_QUADS;

        std::vector <Point3D <double>> trimmedGrid;

        auto fullGrid = GridGenerator <double>::generateQuadGrid(Point3D <double>(
                                                                m_task.bounds.minimumX(),
                                                                m_task.bounds.minimumZ(),
                                                                m_task.bounds.minimumY()
                                                            ),
                                                            m_task.bounds.width(),
                                                            m_task.bounds.length(),
                                                            m_task.cellSize);

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

    }else {
        for (const auto& t : *triangles){
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
