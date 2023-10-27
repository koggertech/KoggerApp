#ifndef SCENEOBJECT_H
#define SCENEOBJECT_H

#include <abstractentitydatafilter.h>
#include <cube.h>

#include <QObject>
#include <QUuid.h>
#include <QColor>
#include <QQmlEngine>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>

#define RENDER_IMPL(Class) ({dynamic_cast<Class##RenderImplementation*>(m_renderImpl);})

class SceneObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(AbstractEntityDataFilter* filter   READ filter                       CONSTANT)
    Q_PROPERTY(QString                   name     READ name       WRITE setName     CONSTANT)
    Q_PROPERTY(SceneObjectType           type     READ type                         CONSTANT)
    Q_PROPERTY(bool                      visible  READ isVisible  WRITE setVisible  CONSTANT)
    Q_PROPERTY(QColor                    color    READ color                        CONSTANT)
    Q_PROPERTY(qreal                     width    READ width                        CONSTANT)

public:
    class RenderImplementation
    {
    public:
        RenderImplementation();
        virtual ~RenderImplementation();

        virtual void render(QOpenGLFunctions *ctx,
                            const QMatrix4x4 &mvp,
                            const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const;

        virtual void setData(const QVector<QVector3D>& data, int primitiveType = GL_POINTS);
        virtual void setColor(QColor color);
        virtual void setWidth(qreal width);
        virtual void setVisible(bool isVisible);
        virtual void clearData();
        QVector<QVector3D> data() const;
        const QVector<QVector3D>& cdata() const;
        QColor color() const;
        qreal width() const;
        bool isVisible() const;
        Cube bounds() const;
        int primitiveType() const;

    protected:
        QVector<QVector3D> m_data;
        QColor m_color = QColor(0.0f, 0.0f, 0.0f);
        float m_width = 1.0f;
        bool m_isVisible = true;
        Cube m_bounds;
        int m_primitiveType = GL_POINTS;

    private:
        virtual void createBounds();

    private:
        friend class SceneObject;
    };

    SceneObject(QObject *parent = nullptr);

    virtual ~SceneObject();

    enum class SceneObjectType{
        BottomTrack  = 0,
        Surface      = 1,
        Point        = 2,
        Polygon      = 3,
        PointGroup   = 5,
        PolygonGroup = 6,
        Unknown      = 7
    };

    Q_ENUM(SceneObjectType)

    QString id() const;
    virtual SceneObjectType type() const;
    QString name() const;
    QVector <QVector3D> data() const;
    const QVector <QVector3D>& cdata() const;
    bool isVisible() const;
    QColor color() const;
    float width() const;
    Cube bounds() const;
    AbstractEntityDataFilter* filter() const;
    int primitiveType() const;
    QVector3D position() const;
    static void qmlDeclare();

protected:
    SceneObject(RenderImplementation* impl,
                QObject *parent = nullptr,
                QString name = QStringLiteral("Scene object"));

public Q_SLOTS:
    /**
     * @brief Sets the name of object
     * @param[in] name - name of object
     */
    void setName(QString name);

    //! @brief Устанавливает набор вершин объекта.
    //! @param[in] data - ссылка на набор вершин.
    virtual void setData(const QVector <QVector3D>& data, int primitiveType = GL_POINTS);

    virtual void clearData();

    void setVisible(bool isVisible);

    void setColor(QColor color);

    void setWidth(qreal width);

    void setFilter(std::shared_ptr <AbstractEntityDataFilter> filter);

Q_SIGNALS:
    void dataChanged();

    void nameChanged(QString name);

    void boundsChanged();

    void filterChanged(AbstractEntityDataFilter* filter);

    void changed();

private:
    friend class GraphicsScene3dView;
    friend class GraphicsScene3dRenderer;
    friend class RenderImplementation;

protected:
    QString m_name = QStringLiteral("Scene object");
    QUuid m_uuid   = QUuid::createUuid();
    std::shared_ptr <AbstractEntityDataFilter> m_filter;
    RenderImplementation* m_renderImpl;
};

#endif // SCENEOBJECT_H
