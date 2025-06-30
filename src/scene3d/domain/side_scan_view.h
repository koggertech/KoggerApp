#pragma once

#include <vector>
#include <QVector>
#include <QVector3D>
#include <QUuid>
#include <QQueue>
#include <QReadWriteLock>
#include "scene_object.h"
#include "dataset.h"
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
    enum class Mode {
        kUndefined = 0,
        kRealtime,
        kPerformance
    };

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
        GLuint colorTableTextureId_;
        GLenum colorTableTextureType_;
    };

    /*methods*/
    explicit SideScanView(QObject* parent = nullptr);
    virtual ~SideScanView();

    bool updateChannelsIds();
    void startUpdateDataInThread(int endIndx, int endOffset = 0);
    void updateData(int endIndx, int endOffset = 0, bool backgroungThread = false);
    void resetTileSettings(int tileSidePixelSize, int tileHeightMatrixRatio, float tileResolution);
    void clear(bool force = true);

    void setView(GraphicsScene3dView* viewPtr);
    void setDatasetPtr(Dataset* datasetPtr);
    void setMeasLineVisible(bool state);
    void setTileGridVisible(bool state);
    void setGenerateGridContour(bool state);
    void setColorTableThemeById(int id);
    void setColorTableLevels(float lowVal, float highVal);
    void setColorTableLowLevel(float val);
    void setColorTableHighLevel(float val);
    void setTextureIdByTileId(QUuid tileId, GLuint textureId);
    void setUseLinearFilter(bool state);
    void setColorTableTextureId(GLuint value);
    void setWorkMode(Mode mode);
    void setLAngleOffset(float val);
    void setRAngleOffset(float val);
    void setChannels(const ChannelId& firstChId, uint8_t firstSubChId, const ChannelId& secondChId, uint8_t secondSubChId);
    GLuint                              getTextureIdByTileId(QUuid tileId);
    bool                                getUseLinearFilter() const;
    GLuint                              getColorTableTextureId() const;
    QHash<QUuid, std::vector<uint8_t>>  getTileTextureTasks();
    std::vector<uint8_t>                getColorTableTextureTask();
    Mode                                getWorkMode() const;

signals:
    void sendStartedInThread(bool);
    void sendUpdatedWorkMode(Mode);

private:
    /*methods*/
    inline bool checkLength(float dist) const;
    MatrixParams getMatrixParams(const QVector<QVector3D> &vertices) const;
    void concatenateMatrixParameters(MatrixParams& srcDst, const MatrixParams& src) const;
    inline int getColorIndx(Epoch::Echogram* charts, int ampIndx) const;
    void postUpdate();
    void updateTilesTexture();
    void updateUnmarkedHeightVertices(Tile* tilePtr) const;

    /*data*/
    static constexpr float amplitudeCoeff_ = 100.0f;
    static constexpr int colorTableSize_ = 255;
    static constexpr int interpLineWidth_ = 1;

    std::vector<uint8_t> colorTableTextureTask_;
    QHash<QUuid, std::vector<uint8_t>> tileTextureTasks_;
    PlotColorTable colorTable_;
    MatrixParams lastMatParams_;
    Dataset* datasetPtr_;
    float tileResolution_;
    uint64_t currIndxSec_;
    ChannelId segFChannelId_;
    uint8_t segFSubChannelId_;
    ChannelId segSChannelId_;
    uint8_t segSSubChannelId_;
    int tileSidePixelSize_;
    int tileHeightMatrixRatio_;
    int lastCalcEpoch_;
    int lastAcceptedEpoch_;
    GlobalMesh globalMesh_;
    bool useLinearFilter_;
    GLuint colorMapTextureId_;
    Mode workMode_;
    bool manualSettedChannels_;
    float lAngleOffset_;
    float rAngleOffset_;
    QMutex mutex_;
    QReadWriteLock rWLocker_;
    bool startedInThread_;
};
