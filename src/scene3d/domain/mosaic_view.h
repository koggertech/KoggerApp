#pragma once

#include <vector>
#include <QVector>
#include <QVector3D>
#include <QUuid>

#include "surface_tile.h"
#include "scene_object.h"


class MosaicView : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MosaicView)

public:
    class MosaicViewRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        MosaicViewRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx,
                            const QMatrix4x4& mvp,
                            const QMap<QString,
                            std::shared_ptr<QOpenGLShaderProgram>>& shaderProgramMap) const override final;
    private:
        friend class MosaicView;

        virtual void updateBounds() override final;

        QHash<QUuid, SurfaceTile> tiles_; // from dataProcessor
        QVector<QVector3D> measLinesVertices_; // from dataProcessor
        QVector<int> measLinesEvenIndices_; // from dataProcessor
        QVector<int> measLinesOddIndices_; // from dataProcessor
        GLuint colorTableTextureId_;
        GLenum colorTableTextureType_;
        bool tileGridVisible_;
        bool measLineVisible_;
    };

    explicit MosaicView(QObject* parent = nullptr);
    virtual ~MosaicView();

    void setTextureIdByTileId(QUuid tileId, GLuint textureId);
    void setColorTableTextureId(GLuint value);
    void setMeasLineVisible(bool state);
    void setTileGridVisible(bool state);
    void setUseLinearFilter(bool state);
    QVector<GLuint>                                 takeVectorTileTextureIdToDelete();
    QVector<std::pair<QUuid, std::vector<uint8_t>>> takeVectorTileTextureToAppend();
    std::vector<uint8_t>                            takeColorTableTextureTask();
    GLuint                                          takeColorTableDeleteTextureId();
    GLuint                             getTextureIdByTileId(QUuid tileId);
    GLuint                             getColorTableTextureId() const;
    bool                               getUseLinearFilter() const;

public slots: // from dataprocessor
    void clear();

    void setTiles(const QHash<QUuid, SurfaceTile>& tiles);
    void setMeasLinesVertices(const QVector<QVector3D>& measLinesVertices);
    void setMeasLinesEvenIndices(const QVector<int>& measLinesEvenIndices);
    void setMeasLinesOddIndices(const QVector<int>& measLinesOddIndices);
    void setColorTableTextureTask(const std::vector<uint8_t>& colorTableTextureTask);

private:
    void updateTileTextureTask(const QHash<QUuid, SurfaceTile>& newTiles);

    std::vector<uint8_t> colorTableTextureTask_; // append
    GLuint colorTableDeleteTextureId_; // delete
    QHash<QUuid, std::vector<uint8_t>> tileTextureTasks_; // append/delete(if val == std::vector<uint8_t>>())
    bool useLinearFilter_;
    QVector<GLuint> vectorTileTextureIdToDelete_;
    QVector<std::pair<QUuid, std::vector<uint8_t>>> vectorTileTextureToAppend_;
};
