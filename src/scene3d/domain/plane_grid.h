#ifndef PLANEGRID_H
#define PLANEGRID_H

#include "scene_object.h"
#include "plane.h"

class PlaneGrid : public SceneObject
{
    Q_OBJECT
public:
    class PlaneGridRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        PlaneGridRenderImplementation();
        virtual ~PlaneGridRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx,
                          const QMatrix4x4& mvp,
                          const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;

        virtual void render(QOpenGLFunctions* ctx,
                            const QMatrix4x4& model,
                            const QMatrix4x4& view,
                            const QMatrix4x4& projection,
                            const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;
    private:
        friend class PlaneGrid;
        QSizeF m_size = {10,10};
        int m_cellSize = 1.0f;
        QVector3D m_position = {0.0f, 0.0f, 0.0f};
    };

    void setPlane(const Plane& plane);
    void setSize(const QSizeF& size);
    void setPosition(const QVector3D& pos);
    void setCellSize(int size);

    explicit PlaneGrid(QObject *parent = nullptr);

public Q_SLOTS:
    virtual void setData(const QVector <QVector3D>& data, int primitiveType = GL_POINTS) override;

private:
    QSizeF m_size = {10,10};
    int m_cellSize = 1.0f;
    QVector3D m_position = {0.0f, 0.0f, 0.0f};
};

#endif // PLANEGRID_H
