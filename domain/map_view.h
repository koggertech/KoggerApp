#pragma once

#include <QString>
#include <QVector3D>
#include <QImage>

#include "sceneobject.h"
#include "map_defs.h"
#include "tile_set.h"


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

        std::shared_ptr<map::TileSet> tileSetPtr_;
    };

    /*methods*/
    explicit MapView(GraphicsScene3dView* view = nullptr, QObject* parent = nullptr);
    virtual ~MapView();
    void clear();
    void setView(GraphicsScene3dView* viewPtr);

    void setTileSetPtr(std::shared_ptr<map::TileSet> ptr);
    void setRectVertices(const QVector<QVector3D>& vertices);

public slots:
    void onTileSetUpdated();


private:

};
