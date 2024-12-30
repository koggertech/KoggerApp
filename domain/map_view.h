#pragma once

#include <QString>
#include <QVector3D>
#include <QImage>
#include <unordered_map>

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

        QVector<QVector3D> firstVertices_;
        QVector<QVector3D> secondVertices_;
        bool isGreen_;
        bool isPerspective_;
        QVector3D point_;
        std::unordered_map<map::TileIndex, map::Tile> tilesHash_;
    };

    /*methods*/
    explicit MapView(QObject* parent = nullptr);
    virtual ~MapView();

    void clear();
    void update();

    void setRectVertices(const QVector<LLA>& vertices, const QVector<LLA>& vertices2, bool green, bool isPerspective_, QVector3D centralPoint);
    void setTextureIdByTileIndx(const map::TileIndex& tileIndx, GLuint textureId);
    std::unordered_map<map::TileIndex, QImage> getInitTileTextureTasks();
    QList<GLuint> getDeinitTileTextureTasks();

public slots:
    void onTileAppend(const map::Tile& tile);
    void onTileDelete(const map::Tile& tile);
    void onTileVerticesUpdated(const map::Tile& tile);
    void onClearAppendTasks();

signals:
    void sendTextureId(const map::TileIndex& tileIndx, GLuint textureId);
    void deleteFromAppend(const map::TileIndex& tileIndx);

private:
    std::unordered_map<map::TileIndex, QImage> appendTasks_;
    QList<GLuint> deleteTasks_;
};
