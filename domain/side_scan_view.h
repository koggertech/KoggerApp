#pragma once

#include <QColor>
#include <QImage>
#include <QVector>
#include "sceneobject.h"
#include "plotcash.h"
#include "surfaceprocessor.h"


class SideScanView : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SideScanView)

public:
    /*structures*/
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

    /*methods*/
    explicit SideScanView(QObject* parent = nullptr);
    virtual ~SideScanView();

    Q_INVOKABLE void updateData(const QString& imagePath, const QString& heightMatrixPath);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void setScaleFactor(int scaleFactor);

    void setDatasetPtr(Dataset* datasetPtr);
    void setProcTask(const SurfaceProcessorTask& task);

public Q_SLOTS:
    Q_INVOKABLE void setSurfaceData(const QVector<QVector3D>& data, int primitiveType = GL_POINTS);

private:
    /*structures*/
    struct MatrixParams {
        MatrixParams() : width(-1), height(-1), minX(0.0f), maxX(0.0f), minY(0.0f), maxY(0.0f) {};
        int width;
        int height;
        float minX;
        float maxX;
        float minY;
        float maxY;
        bool isValid() const {
            if (width == -1 ||
                height == -1 ||
                qFuzzyCompare(minX, maxX) ||
                qFuzzyCompare(minY, maxY)) {
                return false;
            }
            return true;
        }
    };

    /*methods*/
    void updateColorTable();
    void updateChannelsIds();

    void updateColorMatrix(const QVector<QVector3D> &vertices, const QVector<char>& isOdds,
                      const QVector<int>& epochIndxs, int interpLineWidth = 1, bool sideScanLineDrawing = false);
    void updateHeightMatrix();

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

    QVector<QVector3D> surfaceData_;
    int surfacePrimitiveType_;
    SurfaceProcessorTask surfaceProcTask_;
};
