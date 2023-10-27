#ifndef SURFACE_H
#define SURFACE_H

#include <memory>

#include <sceneobject.h>
#include <contour.h>
#include <surfacegrid.h>

class Surface : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Surface)
    Q_PROPERTY(Contour* contour  READ contour CONSTANT)
    Q_PROPERTY(SurfaceGrid* grid READ grid    CONSTANT)

public:
    class SurfaceRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        virtual void render(QOpenGLFunctions* ctx,
                          const QMatrix4x4& mvp,
                          const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;
    private:
        friend class Surface;
        SceneObject::RenderImplementation m_gridRenderImpl;
        SceneObject::RenderImplementation m_contourRenderImpl;
    };

    explicit Surface(QObject* parent = nullptr);
    virtual ~Surface();
    virtual void setData(const QVector<QVector3D>& data, int primitiveType = GL_POINTS) override;
    virtual void clearData() override;
    virtual SceneObjectType type() const override;
    Contour* contour() const;
    SurfaceGrid* grid() const;

private:
    void updateContour();
    void updateGrid();
    void makeTriangleGrid();
    void makeQuadGrid();
    void makeContourFromTriangles();
    void makeContourFromQuads();

private:
    QString m_bottomTrackId = "";
    std::shared_ptr <Contour> m_contour;
    std::shared_ptr <SurfaceGrid> m_grid;
};

#endif // SURFACE_H
