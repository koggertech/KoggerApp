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
    /*structures*/
    class SideScanViewRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        SideScanViewRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx, const QMatrix4x4& mvp,
                            const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>>& shaderProgramMap) const override final;
    private:
        friend class SideScanView;

        /*data*/
        SceneObject::RenderImplementation gridRenderImpl_;
        QVector<QVector3D> measLinesVertices_;
        QVector<int> measLinesEvenIndices_;
        QVector<int> measLinesOddIndices_;
        QVector<QVector3D> heightVertices_;
        QVector<int> heightIndices_;
        QVector<QVector2D> textureVertices_;
        GLuint textureId_;
        bool measLineVisible_;
        bool gridVisible_;
    };

    /*methods*/
    explicit SideScanView(QObject* parent = nullptr);
    virtual ~SideScanView();

    void updateData(bool sideScanLineDrawing, const QString& imagePath);
    void updateDataSec();

    void clear();

    void setDatasetPtr(Dataset* datasetPtr);
    void setTextureId(GLuint textureId);
    void setScaleFactor(int scaleFactor);
    void setMeasLineVisible(bool state);
    void setGridVisible(bool state);

    QImage& getImagePtr();

private:
    /*structures*/
    struct MatrixParams {
        MatrixParams() : unscaledWidth(-1), unscaledHeight(-1), width(-1), height(-1), hMatWidth(-1), hMatHeight(-1), minX(0.0f), maxX(0.0f), minY(0.0f), maxY(0.0f) {};
        int unscaledWidth;
        int unscaledHeight;
        int width;
        int height;
        int hMatWidth;
        int hMatHeight;
        float minX;
        float maxX;
        float minY;
        float maxY;
        bool isValid() const {
            if (unscaledWidth == -1 ||
                unscaledHeight == -1 ||
                width == -1 ||
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
    inline bool checkLength(float dist) const;
    MatrixParams getMatrixParams(const QVector<QVector3D> &vertices) const;
    MatrixParams concatenateMatrixParameters(const MatrixParams& mat1, const MatrixParams& mat2) const;
    inline int getColorIndx(Epoch::Echogram* charts, int ampIndx) const;

    /*data*/
    static constexpr float amplitudeCoeff_ = 100.0f;
    static constexpr int colorTableSize_ = 255;
    static constexpr int heightStep_ = 1;
    static constexpr int interpLineWidth_ = 1;

    QVector<char> isOdds_;
    QVector<int> epochIndxs_;

    QImage image_;

    QVector<QRgb> colorTable_;
    Dataset* datasetPtr_;
    float scaleFactor_;
    int segFChannelId_;
    int segSChannelId_;

    MatrixParams lastMatParams_;
    int lastCalcEpoch_ = 0;
    uint64_t currIndxSec_ = 0;
};
