#pragma once

#include "sceneobject.h"
#include <QVector>
#include "plotcash.h"

struct Point {
    bool isFilled = false;
};

class SideScanView : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SideScanView)

public:
    class SideScanViewRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        SideScanViewRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx, const QMatrix4x4& mvp, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>>& shaderProgramMap) const override final;
    private:
        friend class SideScanView;
        QVector<int> evenIndices_;
        QVector<int> oddIndices_;
    };

    explicit SideScanView(QObject* parent = nullptr);
    virtual ~SideScanView();

    void updateData();
    void clear();
    void setDatasetPtr(Dataset* datasetPtr);

private:
    void updatePixelMatrix(const QVector<QVector3D> &vertices, float scaleFactor);
    QImage pixelMatrixToImage(const QVector<QVector<Point>> &pixelMatrix);
    /*data*/
    const float matrixScaleFactor_ = 5.0f;
    Dataset* datasetPtr_ = nullptr;
    QVector<QVector<Point>> pixelMatrix_;
};
