#pragma once

#include <QColor>
#include <QImage>
#include <QVector>
#include "sceneobject.h"
#include "plotcash.h"


class SideScanView : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SideScanView)

public:
    class SideScanViewRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        SideScanViewRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx, const QMatrix4x4& mvp,
                            const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>>& shaderProgramMap) const override final;
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
    void updateColorTable();
    void updateChannelsIds();
    void updateMatrix(const QVector<QVector3D> &vertices, QVector<char>& isOdds,
                      QVector<int> epochIndxs, float scaleFactor, int interpLineWidth = 1, bool sideScanLineDrawing = false);
    inline int getColorIndx(Epoch::Echogram* charts, int ampIndx) const;
    inline bool checkLength(float dist) const;
    void saveImageToFile(QImage& image, QString& path) const;

    /*data*/
    const float matrixScaleFactor_ = 10.0f;
    const float amplitudeCoeff_ = 100.0f;
    const int lineThickness_ = 5;    
    const int colorTableSize_ = 255;
    QVector<QRgb> colorTable_;

    Dataset* datasetPtr_;
    QImage image_;
    int segFChannelId_;
    int segSChannelId_;
};
