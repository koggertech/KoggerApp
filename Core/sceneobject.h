#ifndef SCENEOBJECT_H
#define SCENEOBJECT_H

#include <QObject>
#include <QUuid.h>
#include <QColor>

class SceneObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString         name      READ name      WRITE setName      NOTIFY nameChanged)
    Q_PROPERTY(float           lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged)
    Q_PROPERTY(bool            visible   READ isVisible WRITE setVisible   NOTIFY visibilityChanged)
    Q_PROPERTY(QColor          color     READ color     WRITE setColor     NOTIFY colorChanged)
    Q_PROPERTY(SceneObjectType type      READ type                         CONSTANT)

public:
    SceneObject(QString name = QStringLiteral("Scene object"),
                QObject *parent = nullptr);

    virtual ~SceneObject();

    enum SceneObjectType {
        BottomTrack,
        Surface,
        Polygon,
        PointSet,
        PolygonSet,
        Unknown
    };

    Q_ENUM(SceneObjectType)

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
     *  @brief Sets object visibility sign
     *  @param[in] isVisible Visibility sign
     */
    void setVisible(bool isVisible);

    /**
     *  @brief Sets color of the object
     *  @param[in] color Color of the object
     */
    void setColor(QColor color);

    /**
     *  @brief Sets line width of the object's contour
     *  @param[in] line width of the object's contour
     */
    void setLineWidth(float width);

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

    /**
     *  @brief Returns object visibility sign
     *  @return Object visibility sign
     */
    bool isVisible() const;

    /**
     *  @brief Returns object color
     *  @return Object color
     */
    QColor color() const;

    /**
     *  @brief Returns object line width
     *  @return Object line width
     */
    float lineWidth() const;

signals:
    /**
     * @brief Emits when object visibility sign was changed
     * @param[out] isVisible Visibility sign
     */
    void visibilityChanged(bool isVisible);

    /**
     * @brief Emits when object color was changed
     * @param[out] color Object color
     */
    void colorChanged(QColor color);

    /**
     * @brief Emits when object name was changed
     * @param[out] name Object name
     */
    void nameChanged(QString name);

    /**
     * @brief Emits when object contour line width was changed
     * @param[out] lineWidth Object's contour line width
     */
    void lineWidthChanged(float lineWidth);

protected:
    QString mName    = QStringLiteral("Scene object"); ///< Object name
    QUuid mUuid      = QUuid::createUuid();            ///< Object unique id
    QColor mColor    = QColor(0, 0, 0);                ///< Object color
    float mLineWidth = 1.0f;                           ///< Contour line width
    bool mIsVisible  = true;                           ///< Object visibility sign
};

#endif // SCENEOBJECT_H
