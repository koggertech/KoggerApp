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
        float minZ_; // from dataprocessor
        float maxZ_; // from dataprocessor
        float surfaceStep_; // from dataprocessor
        int colorIntervalsSize_; // from dataprocessor
        GLuint textureId_;

        bool iVis_ = false;
        bool mVis_ = false;
    };

    explicit SurfaceView(QObject* parent = nullptr);
    virtual ~SurfaceView();

    void   setMosaicTextureIdByTileId(QUuid tileId, GLuint textureId);
    void   setMosaicColorTableTextureId(GLuint value);
    GLuint getMosaicTextureIdByTileId(QUuid tileId);
    GLuint getMosaicColorTableTextureId() const;

    QVector<GLuint>                                 takeMosaicTileTextureToDelete();
    QVector<std::pair<QUuid, std::vector<uint8_t>>> takeMosaicTileTextureToAppend();
    std::vector<uint8_t>                            takeMosaicColorTableToAppend();
    GLuint                                          takeMosaicColorTableToDelete();
    QVector<uint8_t>                                takeSurfaceColorTableToAppend();
    GLuint                                          takeSurfaceColorTableToDelete();

    GLuint getSurfaceColorTableTextureId() const;
    void setSurfaceColorTableTextureId(GLuint textureId);

    void setIVisible(bool state);
    void setMVisible(bool state);;


public slots: // from dataprocessor
    void clear();
    void setTiles(const QHash<QUuid, SurfaceTile>& tiles, bool useTextures); // TODO: separate (now from mosaic)
    void setMosaicColorTableTextureTask(const std::vector<uint8_t>& colorTableTextureTask);

    void setMinZ(float minZ);
    void setMaxZ(float maxZ);

    void setSurfaceStep(float surfaceStep);
    void setTextureTask(const QVector<uint8_t>& textureTask);
    void setColorIntervalsSize(int size);

private:
    void updateMosaicTileTextureTask(const QHash<QUuid, SurfaceTile>& newTiles);

private:
    std::vector<uint8_t>                            mosaicColorTableToAppend_;
    GLuint                                          mosaicColorTableToDelete_;
    QVector<std::pair<QUuid, std::vector<uint8_t>>> mosaicTileTextureToAppend_;
    QVector<GLuint>                                 mosaicTileTextureToDelete_;

    QVector<uint8_t> textureTask_;
    GLuint toDeleteId_;


};
