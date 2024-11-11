#pragma once

#include <QString>
#include <QVector3D>
#include <QImage>
#include <unordered_map>
#include <QReadWriteLock>

#include "sceneobject.h"
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
        MapViewRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx,
                            const QMatrix4x4& model,
                            const QMatrix4x4& view,
                            const QMatrix4x4& projection,
                            const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override final;

    private:
        friend class MapView;
        QVector<QVector3D> rectVertices_;
        std::unordered_map<map::TileIndex, map::Tile> tilesHash_;
    };

    /*methods*/
    explicit MapView(GraphicsScene3dView* view = nullptr, QObject* parent = nullptr);
    virtual ~MapView();

    void clear();

    void setView(GraphicsScene3dView* viewPtr);
    void setRectVertices(const QVector<QVector3D>& vertices);
    void setTextureIdByTileIndx(const map::TileIndex& tileIndx, GLuint textureId);

    std::unordered_map<map::TileIndex, QImage> getInitTileTextureTasks();
    QList<GLuint> getDeinitTileTextureTasks();

public slots:
    void onTileAppend(const map::Tile& tile);
    void onTileDelete(const map::Tile& tile);

signals:
    void updatedTextureId(const map::TileIndex& tileIndx, GLuint textureId);

private:
    std::unordered_map<map::TileIndex, QImage> appendTasks_;
    QList<GLuint> deleteTasks_;
    QReadWriteLock rWLocker_;
};
