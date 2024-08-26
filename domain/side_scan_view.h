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

    Q_INVOKABLE void updateData(const QString& imagePath, const QString& heightMatrixPath);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void setScaleFactor(float scaleFactor);
    void setDatasetPtr(Dataset* datasetPtr);

private:
    struct MatrixParams {
        MatrixParams() : width_(-1), height_(-1), minX(0.0f), maxX(0.0f), minY(0.0f), maxY(0.0f) {};
        int width_;
        int height_;
        float minX;
        float maxX;
        float minY;
        float maxY;
        bool isValid() const {
            if (width_ == -1 ||
                height_ == -1 ||
                qFuzzyCompare(minX, maxX) ||
                qFuzzyCompare(minY, maxY)) {
                return false;
            }
            return true;
        }
    };

    void updateColorTable();
    void updateChannelsIds();
    void updateColorMatrix(const QVector<QVector3D> &vertices, const QVector<char>& isOdds,
                      const QVector<int>& epochIndxs, int interpLineWidth = 1, bool sideScanLineDrawing = false);
    void updateHeightMatrix(const QVector<QVector3D> &vertices,const QVector<int>& epochIndxs);

    MatrixParams getMatrixParams(const QVector<QVector3D> &vertices);
    inline int getColorIndx(Epoch::Echogram* charts, int ampIndx) const;
    inline bool checkLength(float dist) const;
    void saveImageToFile(const QString& path) const;
    void writeHeightMatrixToFile(const QString& path) const;
    void readHeightMatrixFromFile(const QString& path);

    /*data*/
    const float amplitudeCoeff_ = 100.0f;
    const int lineThickness_ = 5;    
    const int colorTableSize_ = 255;
    QVector<QRgb> colorTable_;

    Dataset* datasetPtr_;
    MatrixParams matParams_;
    QImage image_;
    QVector<QVector<float>> heightMatrix_;
    float scaleFactor_;
    int segFChannelId_;
    int segSChannelId_;
};
