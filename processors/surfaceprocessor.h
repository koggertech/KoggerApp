#ifndef SURFACEPROCESSOR_H
#define SURFACEPROCESSOR_H

#include <memory>

#include <QObject>

#include <vertexobject.h>

class SurfaceProcessor : public QObject
{
    Q_OBJECT
public:
    explicit SurfaceProcessor(QObject *parent = nullptr);

    void process(QVector <QVector3D> sourceData,
                 std::shared_ptr <VertexObject> surface,
                 bool needSmoothing = false,
                 float cellSize = 5.0);

signals:

    void processingStarted();

    void processingFinished();

};

#endif // SURFACEPROCESSOR_H
