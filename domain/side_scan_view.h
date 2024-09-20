#pragma once

#include <QVector>
#include <QVector3D>
#include <QImage>
#include <QPair>
#include <QUuid>
#include <QQueue>
#include "sceneobject.h"
#include "plotcash.h"
#include "global_mesh.h"
#include "tile.h"
#include "draw_utils.h"


using namespace sscan;

class GraphicsScene3dView;
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
        QHash<QUuid, Tile> tiles_;
        QVector<QVector3D> measLinesVertices_;
        QVector<int> measLinesEvenIndices_;
        QVector<int> measLinesOddIndices_;
        bool tileGridVisible_;
        bool measLineVisible_;
    };

    /*methods*/
    explicit SideScanView(QObject* parent = nullptr);
    virtual ~SideScanView();

    bool updateChannelsIds();
    void updateData(int endOffset = 0);
    void resetTileSettings(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution);
    void clear();

    void setView(GraphicsScene3dView* viewPtr);
    void setDatasetPtr(Dataset* datasetPtr);
    void setMeasLineVisible(bool state);
    void setTileGridVisible(bool state);
    void setGenerateGridContour(bool state);
    void setColorTableThemeById(int id);
    void setColorTableLowLevel(int val);
    void setColorTableHighLevel(int val);


    void setTextureIdByTileId(QUuid tileId, GLuint textureId);
    GLuint  getTextureIdByTileId(QUuid tileId);

    void setUseLinearFilter(bool state);
    bool getUseLinearFilter() const;

    void setTrackLastEpoch(bool state);
    bool getTrackLastEpoch() const;

    QVector<QRgb> getColorTable() const;
    QHash<QUuid, QImage>& getProcessTextureTasksRef();

private:
    /*methods*/
    inline bool checkLength(float dist) const;
    MatrixParams getMatrixParams(const QVector<QVector3D> &vertices) const;
    void concatenateMatrixParameters(MatrixParams& srcDst, const MatrixParams& src) const;
    inline int getColorIndx(Epoch::Echogram* charts, int ampIndx) const;
    void postUpdate();
    void updateTilesTexture();

    /*data*/
    static constexpr float amplitudeCoeff_ = 100.0f;
    static constexpr int colorTableSize_ = 255;
    static constexpr int interpLineWidth_ = 1;

    QHash<QUuid, QImage> processTextureTasks_;
    PlotColorTable colorTable_;
    MatrixParams lastMatParams_;
    Dataset* datasetPtr_;
    float tileResolution_;
    uint64_t currIndxSec_;
    int segFChannelId_;
    int segSChannelId_;
    int tileSidePixelSize_;
    int tileHeightMatrixRatio_;
    int lastCalcEpoch_;
    int lastAcceptedEpoch_;
    GlobalMesh globalMesh_;
    bool useLinearFilter_;
    bool trackLastEpoch_;
};
