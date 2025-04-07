#ifndef POLYGONGROUP_H
#define POLYGONGROUP_H

#include <QStandardItemModel>

#include "polygon_object.h"

class PolygonGroup : public SceneObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PolygonGroup)

public:
    class PolygonGroupRenderImplementation : public SceneObject::RenderImplementation
    {
      public:
        PolygonGroupRenderImplementation();
        virtual ~PolygonGroupRenderImplementation();
        virtual void render(QOpenGLFunctions* ctx,
                            const QMatrix4x4& mvp,
                            const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;
        void appendPolygonRenderImpl(PolygonObject::PolygonObjectRenderImplementation* impl);
         void removeAt(int index);
    private:
        friend class PolygonGroup;

    protected:
        QList <PolygonObject::PolygonObjectRenderImplementation> m_polygonRenderImplList;
    };

    explicit PolygonGroup(QObject *parent = nullptr);
    virtual ~PolygonGroup();
    virtual SceneObject::SceneObjectType type() const override;
    std::shared_ptr <PolygonObject> at(int index) const;
    Q_INVOKABLE PolygonObject* polygonAt(int index);

public Q_SLOTS:
    void addPolygon(std::shared_ptr <PolygonObject> polygon);
    void removePolygon(std::shared_ptr <PolygonObject> polygon);
    void removePolygonAt(int index);
    void setData(const QVector<QVector3D>& data, int primitiveType = GL_POINTS) override;
    void clearData() override;

private Q_SLOTS:
    void polygonObjectChanged();

private:
    QList <std::shared_ptr <PolygonObject>> m_polygonList;
    int m_indexInGroup = 0;
};

#endif // POLYGONGROUP_H
