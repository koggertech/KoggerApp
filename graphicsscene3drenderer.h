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
#include "qsystemdetection.h"
#if !defined(Q_OS_ANDROID)
#include <GL/gl.h>
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <QOpenGLTexture>
#else
// #include <QOpenGLFunctions_3_0>
#include <QOpenGLExtraFunctions>
#include <GLES2/gl2.h>
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>
#include <GLES/gl.h>
#endif

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
    GLuint VAO, VBO;
};

#endif // GRAPHICSSCENE3D_H
