#ifndef GRAPHICSSCENE3D_H
#define GRAPHICSSCENE3D_H

#include <coordinateaxes.h>
#include <planegrid.h>
#include <bottomtrack.h>
#include <surface.h>
#include <pointgroup.h>
#include <polygongroup.h>
#include <graphicsscene3dview.h>
#include <geometryengine.h>

#include <QMatrix4x4>
#include <QOpenGLExtraFunctions>
#include <QOpenGLTexture>
#include <QVector2D>
#include <QMutex>

#include <memory>

class QOpenGLShaderProgram;
class GraphicsScene3dRenderer : protected QOpenGLExtraFunctions
{
public:
    GraphicsScene3dRenderer();
    virtual ~GraphicsScene3dRenderer();
    void render();

private:
    void initialize();
    void drawObjects();
    QMatrix4x4 model() const;
    QMatrix4x4 view() const;
    QMatrix4x4 projection() const;

protected:

    QMap <QString, std::shared_ptr <QOpenGLShaderProgram>> m_shaderProgramMap;
    bool m_isInitialized = false;

private:
    struct Character
    {
        QOpenGLTexture* texture;
        GLuint    textureId; // ID текстуры глифа
        QVector2D size;      // Размеры глифа
        QVector2D bearing;   // Смещение верхней левой точки глифа
        GLuint    advance;   // Горизонтальное смещение до начала следующего глифа
    };

    void initFont();
    void doTexture();
    void displayTexture(const QMatrix4x4& model,
                        const QMatrix4x4& view,
                        const QMatrix4x4& projection);

private:

    friend class GraphicsScene3dView;
    friend class InFboRenderer;

    QSizeF m_viewSize;
    GraphicsScene3dView::Camera m_camera;
    GraphicsScene3dView::Camera m_axesThumbnailCamera;
    CoordinateAxes::CoordinateAxesRenderImplementation m_coordAxesRenderImpl;
    PlaneGrid::PlaneGridRenderImplementation m_planeGridRenderImpl;
    Surface::SurfaceRenderImplementation m_surfaceRenderImpl;
    BottomTrack::BottomTrackRenderImplementation m_bottomTrackRenderImpl;
    PolygonGroup::PolygonGroupRenderImplementation m_polygonGroupRenderImpl;
    PointGroup::PointGroupRenderImplementation m_pointGroupRenderImpl;
    SceneObject::RenderImplementation m_boatTrackRenderImpl;

    QMatrix4x4 m_model;
    QMatrix4x4 m_projection;
    QRect m_comboSelectionRect;
    Cube m_boundingBox;
    float m_verticalScale = 1.0f;
    bool m_isSceneBoundingBoxVisible = true;

    QHash<int,Character> m_characters;
    GLuint VAO, VBO;

    std::unique_ptr<QOpenGLTexture> m_texture;
    std::unique_ptr<GeometryEngine> m_geometryEngine;
};

#endif // GRAPHICSSCENE3D_H
