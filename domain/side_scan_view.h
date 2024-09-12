#pragma once

#include <QColor>
#include <QImage>
#include <QVector>
#include "sceneobject.h"
#include "plotcash.h"

#include "global_mesh.h"
#include "tile.h"
#include "drawutils.h"


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
        virtual void createBounds() override final;

        /*data*/
        // tiles
        QHash<QUuid, Tile> tiles_;

        // meas lines
        QVector<QVector3D> measLinesVertices_;
        QVector<int> measLinesEvenIndices_;
        QVector<int> measLinesOddIndices_;
        bool measLineVisible_;
    };

    /*methods*/
    explicit SideScanView(QObject* parent = nullptr);
    virtual ~SideScanView();

    void updateDataSec();
    void clear();

    void setDatasetPtr(Dataset* datasetPtr);
    void setTextureId(GLuint textureId);
    void setScaleFactor(int scaleFactor);
    void setMeasLineVisible(bool state);
    void setGridVisible(bool state);

    QImage& getImagePtr();
    void updateChannelsIds();

private:
    /*methods*/
    void updateColorTable();
    inline bool checkLength(float dist) const;
    MatrixParams getMatrixParams(const QVector<QVector3D> &vertices) const;
    void concatenateMatrixParameters(MatrixParams& srcDst1, const MatrixParams& src2) const;
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

    GlobalMesh  globalMesh_;
    QVector3D  origin_;



    MatrixParams lastMatParams_;
    int lastCalcEpoch_ = 0;
    uint64_t currIndxSec_ = 0;
};
