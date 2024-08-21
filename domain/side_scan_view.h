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
    void updateMatrix(const QVector<QVector3D> &vertices, QVector<char>& isOdds, QVector<int> epochIndxs, float scaleFactor, int interpLineWidth = 1, bool sideScanLineDrawing = false);
    QImage getImageFromMatrix() const;
    void saveImageToFile(QImage& image, QString& path) const;
    void updateChannelsIds();
    QColor getFixedColor(Epoch::Echogram* charts, int ampIndx) const;
    bool checkLength(float dist) const;

    /*data*/
    const float matrixScaleFactor_ = 20.0f;
    const float amplitudeCoeff_ = 100.0f;
    const int lineThickness_ = 5;

    Dataset* datasetPtr_;
    QVector<QVector<Point>> pixelMatrix_;
    int segFChannelId_;
    int segSChannelId_;
};
