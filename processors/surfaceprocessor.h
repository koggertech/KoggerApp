#ifndef SURFACEPROCESSOR_H
#define SURFACEPROCESSOR_H

#include <GL/gl.h>
#include <memory>
#include <atomic>

#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QThread>

#include <cube.h>

class SurfaceProcessor : public QObject
{
    Q_OBJECT

public:
    struct Task{
        QVector <QVector3D> source;
        Cube bounds;
        bool needSmoothing = false;
        qreal cellSize = 5.0f;
    };

    struct Result{
        QVector <QVector3D> data;
        int primitiveType = GL_TRIANGLES;
    };

    explicit SurfaceProcessor(QObject *parent = nullptr);
    virtual ~SurfaceProcessor();

    bool setTask(const Task& task);
    bool startInThread();
    bool startInThread(const Task& task);
    bool stopInThread(unsigned long time = ULONG_MAX);

    bool isBusy() const;
    Result result() const;

private Q_SLOTS:
    void process();

Q_SIGNALS:
    void taskStarted();
    void taskFinished(Result result);

private:
    Task m_task;
    Result m_result;
    std::atomic_bool m_isBusy{false};
};

#endif // SURFACEPROCESSOR_H
