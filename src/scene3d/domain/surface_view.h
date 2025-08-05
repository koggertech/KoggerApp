#pragma once

#include <vector>
#include <QVector>
#include <QVector3D>
#include <QUuid>

#include "surface_tile.h"
#include "scene_object.h"


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
        friend class SurfaceView;

        QHash<QUuid, SurfaceTile> tiles_; // from dataProcessor
        GLuint mosaicColorTableTextureId_;
        GLenum mosaicColorTableTextureType_;
    };

    explicit SurfaceView(QObject* parent = nullptr);
    virtual ~SurfaceView();

    void   setMosaicTextureIdByTileId(QUuid tileId, GLuint textureId);
    void   setMosaicColorTableTextureId(GLuint value);
    GLuint getMosaicTextureIdByTileId(QUuid tileId);
    GLuint getMosaicColorTableTextureId() const;

    QVector<GLuint>                                 takeMosaicVectorTileTextureIdToDelete();
    QVector<std::pair<QUuid, std::vector<uint8_t>>> takeMosaicVectorTileTextureToAppend();
    std::vector<uint8_t>                            takeMosaicColorTableTextureTask();
    GLuint                                          takeMosaicColorTableDeleteTextureId();

public slots: // from dataprocessor
    void clear();
    void setTiles(const QHash<QUuid, SurfaceTile>& tiles); // TODO: separate (now from mosaic)
    void setMosaicColorTableTextureTask(const std::vector<uint8_t>& colorTableTextureTask);

private:
    void updateMosaicTileTextureTask(const QHash<QUuid, SurfaceTile>& newTiles);

private:
    std::vector<uint8_t>                            mosaicColorTableToAppend_;
    GLuint                                          mosaicColorTableToDelete_;
    QVector<std::pair<QUuid, std::vector<uint8_t>>> mosaicTileTextureToAppend_;
    QVector<GLuint>                                 mosaicTileTextureToDelete_;

};
