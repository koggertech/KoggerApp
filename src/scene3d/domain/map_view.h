#pragma once

#include <QString>
#include <QVector3D>
#include <QImage>
#include <unordered_map>

#include "scene_object.h"
#include "map_defs.h"


class GraphicsScene3dView;
class MapView : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MapView)

public:
    /*structures*/
    class MapViewRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        struct InitTask
        {
            map::TileIndex idx;
            QImage img;
        };

        struct UpdateTask {
            map::TileIndex idx;
            QImage img;
        };

        MapViewRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx,
                            const QMatrix4x4& model,
                            const QMatrix4x4& view,
                            const QMatrix4x4& projection,
                            const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override final;

        void copyCpuSideFrom(const MapViewRenderImplementation& s); // copy cpu data

    private:
        friend class GraphicsScene3dView;
        friend class MapView;

        mutable QVector<InitTask>       pendingInit_;
        mutable QVector<UpdateTask>     pendingUpdate_;
        mutable QVector<map::TileIndex> pendingDelete_;

        mutable GLuint vboPos_ = 0;
        mutable GLuint vboUV_  = 0;
        mutable std::unordered_map<map::TileIndex, map::Tile> tilesHash_;

        void processPendingTextureTasks(QOpenGLFunctions* gl) const;
        void ensureQuadBuffers(QOpenGLFunctions* gl) const;
    };

    /*methods*/
    explicit MapView(QObject* parent = nullptr);
    virtual ~MapView();

    void clear();
    void update();

    std::unordered_map<map::TileIndex, QImage> takeInitTileTasks();
    std::unordered_map<map::TileIndex, QImage> takeUpdateTileTasks();
    QVector<map::TileIndex>                    takeDeleteTileTasks();

public slots:
    void onTileAppend(const map::Tile& tile);
    void onTileDelete(const map::TileIndex& tileIndx);
    void onTileImageUpdated(const map::TileIndex& tileIndx, const QImage& image);
    void onTileVerticesUpdated(const map::TileIndex& tileIndx, const QVector<QVector3D>& vertices);
    void onClearAppendTasks();//

signals:
    void sendTextureId(const map::TileIndex& tileIndx, GLuint textureId);//
    void deletedFromAppend(const map::TileIndex& tileIndx);//

private:
    std::unordered_map<map::TileIndex, QImage> appendTasks_;
    std::unordered_map<map::TileIndex, QImage> updateImageTasks_;
    QVector<map::TileIndex> deleteTasks_;
};
