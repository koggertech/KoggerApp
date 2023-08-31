#ifndef SCENEOBJECT_H
#define SCENEOBJECT_H

#include <QObject>
#include <QUuid.h>
#include <QColor>
#include <QQmlEngine>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>

class SceneObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString         name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(SceneObjectType type READ type               CONSTANT)

public:

    explicit SceneObject(QObject *parent = nullptr);

    SceneObject(QString name = QStringLiteral("Scene object"),
                QObject *parent = nullptr);

    virtual ~SceneObject();

    enum class SceneObjectType{
        BottomTrack  = 0,
        Surface      = 1,
        Point        = 2,
        Polygon      = 3,
        ObjectsGroup = 4,
        PointGroup   = 5,
        PolygonGroup = 6,
        Unknown      = 7
    };

    Q_ENUM(SceneObjectType)

    virtual void draw(QOpenGLFunctions* ctx,
                      const QMatrix4x4& mvp,
                       QMap <QString, QOpenGLShaderProgram*> shaderProgramMap) const;

    /**
     * @brief Returns type of the object
     * @return type of the object
     */
    virtual SceneObjectType type() const;

    /**
     * @brief Sets the name of object
     * @param[in] name - name of object
     */
    void setName(QString name);

    /**
     * @brief Returns unique id of the object
     * @return unique id of the object
     */
    QString id() const;

    /**
     * @brief Returns name of the object
     * @return name of the object
     */
    QString name() const;

    static void qmlDeclare();

signals:

    /**
     * @brief Emits when object name was changed
     * @param[out] name Object name
     */
    void nameChanged(QString name);

protected:
    QString mName = QStringLiteral("Scene object"); ///< Object name
    QUuid mUuid   = QUuid::createUuid();            ///< Object unique id
};

#endif // SCENEOBJECT_H
