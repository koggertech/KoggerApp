#pragma once

#include <QString>
#include <QVector3D>
#include <QImage>

#include "sceneobject.h"
#include "map_utils.h"


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
        QVector<QVector3D> vec_;
        QVector<map::Tile> tiles_;
    };

    /*methods*/
    explicit MapView(GraphicsScene3dView* view = nullptr, QObject* parent = nullptr);
    virtual ~MapView();
    void clear();
    void setView(GraphicsScene3dView* viewPtr);

    void setVec(const QVector<QVector3D>& vec);
    void setTiles(const QVector<map::Tile>& tiles);
private:

};
