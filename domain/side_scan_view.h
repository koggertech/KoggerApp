#pragma once

#include "sceneobject.h"
#include <QVector>
#include <QColor>
#include "plotcash.h"

struct Point {
    QColor color;
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
    void updatePixelMatrix(const QVector<QVector3D> &vertices, float scaleFactor, int lineThickness);
    QImage pixelMatrixToImage(const QVector<QVector<Point>> &pixelMatrix);
    /*data*/
    const float matrixScaleFactor_ = 20.0f;
    const int lineThickness_ = 5;

    Dataset* datasetPtr_ = nullptr;
    QVector<QVector<Point>> pixelMatrix_;

    QVector<bool> isOdd_; // for left/right echograms
    QVector<int> usedIndx_; // for epoch
};
