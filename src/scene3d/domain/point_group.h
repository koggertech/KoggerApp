#ifndef POINTGROUP_H
#define POINTGROUP_H

#include "scene_object.h"
#include "point_object.h"

class PointGroup : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PointGroup)

public:
    class PointGroupRenderImplementation : public SceneObject::RenderImplementation
    {
    public:
        PointGroupRenderImplementation();
        virtual ~PointGroupRenderImplementation();
        virtual void render(QOpenGLFunctions *ctx,
                            const QMatrix4x4 &mvp,
                            const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;

        virtual void clearData() override;
        void appendPointRenderImpl(PointObject::PointObjectRenderImplementation* impl);
        void removeRenderAt(int index);
    private:
        virtual void createBounds() override;

    protected:
        QList <PointObject::PointObjectRenderImplementation> m_pointRenderImplList;

    private:
        friend class PointGroup;
    };

    explicit PointGroup(QObject *parent = nullptr);
    PointGroup(SceneObject::RenderImplementation* impl, QObject *parent = nullptr);
    virtual ~PointGroup();
    virtual SceneObject::SceneObjectType type() const override;
    std::shared_ptr <PointObject> at(int index) const;

public Q_SLOTS:
    virtual void append(std::shared_ptr <PointObject> point);
    virtual void setData(const QVector <QVector3D>& data, int primitiveType = GL_POINTS) override;
    virtual void clearData() override;
    void removeAt(int index);

private:
    void pointObjectChanged();

protected:
    QList <std::shared_ptr <PointObject>> m_pointList;
};

#endif // POINTGROUP_H
