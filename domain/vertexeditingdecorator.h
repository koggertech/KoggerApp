#ifndef VERTEXEDITINGDECORATOR_H
#define VERTEXEDITINGDECORATOR_H

#include <sceneobject.h>

class VertexEditingDecorator : public SceneObject
{
    Q_OBJECT
public:
    class VertexEditingDecoratorRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        VertexEditingDecoratorRenderImplementation();
        virtual ~VertexEditingDecoratorRenderImplementation();

        virtual void render(QOpenGLFunctions *ctx,
                            const QMatrix4x4 &mvp,
                            const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const;
    private:
        friend class VertexEditingDecorator;
    };

    explicit VertexEditingDecorator(QObject *parent = nullptr);
    VertexEditingDecorator(std::weak_ptr<SceneObject> decoratedObject,
                           int decoratedVertexIndex,
                           QObject *parent = nullptr);
    virtual ~VertexEditingDecorator();

    bool isValid() const;

public Q_SLOTS:
    void setDecorated(std::weak_ptr<SceneObject> decoratedObject, int decoratedVertexIndex);
    void moveVertex(QVector3D pos);
    void removeVertex(bool decorateNextVertex = true);
    virtual void clearData() override;

private:
    std::weak_ptr<SceneObject> m_decoratedObject;
    int m_decoratedVertexIndex = 0;
    bool m_isValid = false;
};

#endif // VERTEXEDITINGDECORATOR_H
