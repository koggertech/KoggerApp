#pragma once

#include <vector>
#include <QMutex>
#include <QVector>
#include <QVector3D>
#include "surface_tile.h"
#include "scene_object.h"
#include "dataset_defs.h"


class SurfaceView : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SurfaceView)

public:
    class SurfaceViewRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        SurfaceViewRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx,
                            const QMatrix4x4& mvp,
                            const QMap<QString,
                            std::shared_ptr<QOpenGLShaderProgram>>& shaderProgramMap) const override final;
    private:
        void updateBounds() override final;

        friend class SurfaceView;

        QHash<TileKey, SurfaceTile> tiles_; // from dataProcessor
        GLuint surfaceColorTableTextureId_;
        GLuint mosaicColorTableTextureId_;
        float minZ_; // from dataprocessor
        float maxZ_; // from dataprocessor
        float surfaceStep_; // from dataprocessor
        int colorIntervalsSize_; // from dataprocessor
        bool iVis_;
        bool mVis_;
        QVector<QVector3D> lastLeftLine_;
        QVector<QVector3D> lastRightLine_;
        float traceWidth_;
        bool  traceVisible_;
    };

    explicit SurfaceView(QObject* parent = nullptr);
    virtual ~SurfaceView();

    void   setMosaicTextureIdByTileId(const TileKey& tileId, GLuint textureId);
    void   setMosaicColorTableTextureId(GLuint value);
    void   setSurfaceColorTableTextureId(GLuint textureId);
    void   setIVisible(bool state);
    void   setMVisible(bool state);
    GLuint getMosaicTextureIdByTileId(const TileKey& tileId) const;
    GLuint getMosaicColorTableTextureId() const;
    GLuint getSurfaceColorTableTextureId() const;
    bool   getMVisible() const;
    bool   getIVisible() const;

    QVector<GLuint>                                   takeMosaicTileTextureToDelete();
    QVector<std::pair<TileKey, std::vector<uint8_t>>> takeMosaicTileTextureToAppend();
    std::vector<uint8_t>                              takeMosaicColorTableToAppend();
    GLuint                                            takeMosaicColorTableToDelete();
    std::vector<uint8_t>                              takeSurfaceColorTableToAppend();
    GLuint                                            takeSurfaceColorTableToDelete();

    void setLlaRef(LLARef llaRef);
    void saveVerticesToFile(const QString& path);

    bool trySetMosaicTextureId(const TileKey& key, GLuint texId);
    bool hasTile(const TileKey& key) const;

    void setTraceLines(const QVector3D& leftBeg, const QVector3D& leftEnd, const QVector3D& rightBeg, const QVector3D& rightEnd);

public slots: // from dataprocessor
    void clear();
    void setTiles(const QHash<TileKey, SurfaceTile>& tiles, bool useTextures); // TODO: separate (now from mosaic)
    void setTilesIncremental(const QHash<TileKey, SurfaceTile>& tiles, const QSet<TileKey>& fullVisibleNow /*все видимые на кадре*/);
    void setMosaicColorTableTextureTask(const std::vector<uint8_t>& colorTableTextureTask);
    void setMinZ(float minZ);
    void setMaxZ(float maxZ);
    void setSurfaceStep(float surfaceStep);
    void setTextureTask(const std::vector<uint8_t>& textureTask);
    void setColorIntervalsSize(int size);
    void removeTiles(const QSet<TileKey>& ids);

private:
    void updateMosaicTileTextureTask(const QHash<TileKey, SurfaceTile>& newTiles);

private:
    QMutex mosaicTexTasksMutex_;

    std::vector<uint8_t>                            mosaicColorTableToAppend_;
    GLuint                                          mosaicColorTableToDelete_;
    QHash<TileKey, std::vector<uint8_t>>            mosaicTileTextureToAppend_; // по ключу хранится последнее изображение
    QVector<GLuint>                                 mosaicTileTextureToDelete_;
    std::vector<uint8_t>                            surfaceColorTableToAppend_;
    GLuint                                          surfaceColorTableToDelete_;
    LLARef llaRef_;
};
