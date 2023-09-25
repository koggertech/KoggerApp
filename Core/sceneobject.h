#ifndef SCENEOBJECT_H
#define SCENEOBJECT_H

#include <QObject>
#include <QUuid.h>
#include <QColor>
#include <QQmlEngine>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>

#include <abstractentitydatafilter.h>
#include <cube.h>

class GraphicsScene3d;
class SceneObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(AbstractEntityDataFilter* filter READ filter                 CONSTANT)
    Q_PROPERTY(QString                   name   READ name   WRITE setName   NOTIFY nameChanged)
    Q_PROPERTY(SceneObjectType           type   READ type                   CONSTANT)

public:
    SceneObject(QObject *parent = nullptr);

    SceneObject(QString name = QStringLiteral("Scene object"),
                int primitiveType = GL_POINTS,
                QObject *parent = nullptr);

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

    GraphicsScene3d* scene() const;

    /**
     * @brief Returns unique id of the object
     * @return unique id of the object
     */
    QString id() const;

    /**
     * @brief Returns type of the object
     * @return type of the object
     */
    virtual SceneObjectType type() const;

    /**
     * @brief Returns name of the object
     * @return name of the object
     */
    QString name() const;

    //! @brief Возвращает копию набора вершин объекта.
    //! @return Копмя набора вершин объекта.
    QVector <QVector3D> data() const;

    //! @brief Возвращает копию набора вершин объекта.
    //! @return Копмя набора вершин объекта.
    const QVector <QVector3D>& cdata() const;

    //! @brief Возвращает тип примитива для отображения в движке openGL
    //! (из набора дефайнов gl.h).
    //! @return[in] Тип примитива для отображения в движке openGL
    //! (из набора дефайнов gl.h).
    int primitiveType() const;

    Cube boundingBox() const;

    AbstractEntityDataFilter* filter() const;

    static void qmlDeclare();

protected:
    void createBoundingBox();

public Q_SLOTS:
    /**
     * @brief Sets the name of object
     * @param[in] name - name of object
     */
    void setName(QString name);

    void setScene(GraphicsScene3d* scene);

    //! @brief Устанавливает набор вершин объекта.
    //! @param[in] data - ссылка на набор вершин.
    virtual void setData(const QVector <QVector3D>& data);

    virtual void clearData();

    virtual void setPrimitiveType(int primitiveType);

    void setFilter(std::shared_ptr <AbstractEntityDataFilter> filter);

    //! @brief Добавляет вершину в конец набора вершин.
    //! @param[in] vertex - ссылка на вершину
    virtual void append(const QVector3D& vertex);

    //! @brief Добавляет входящий набор вершин в конец набора вершин объекта
    //! @param[in] other - ссылка на набор вершин
    virtual void append(const QVector<QVector3D>& other);

Q_SIGNALS:
    void dataChanged();

    void nameChanged(QString name);

    void boundsChanged();

    void filterChanged(AbstractEntityDataFilter* filter);

protected:
    GraphicsScene3d* mp_scene;
    QString m_name = QStringLiteral("Scene object"); ///< Object name
    QUuid m_uuid   = QUuid::createUuid();            ///< Object unique id
    QVector <QVector3D> m_data;
    Cube m_boundingBox;                             ///< Bounding cube of the object
    int m_primitiveType = GL_POINTS;
    std::shared_ptr <AbstractEntityDataFilter> m_filter;
};

#endif // SCENEOBJECT_H
