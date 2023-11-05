#ifndef RAYCASTER_H
#define RAYCASTER_H

#include <memory>

#include <QObject>
#include <QVector3D>

class SceneObject;
class RayCasterHit
{
public:
    std::weak_ptr <SceneObject> sourceObject() const;
    std::pair <QVector <QVector3D>, int> sourcePrimitive() const;
    QVector3D worldIntersection() const;
    QPair<int, int> indices() const;
    void setSourceObject(std::weak_ptr <SceneObject> object);
    void setSourcePrimitive(std::pair <QVector <QVector3D>, int> primitive);
    void setWorldIntersection(const QVector3D& intersection);
    void setIndices(int begin, int end);

private:
    std::weak_ptr <SceneObject> m_sourceObject;
    std::pair <QVector <QVector3D>, int> m_sourcePrimitive;
    QVector3D m_worldIntersection;
    QPair <int, int> m_indices = {-1, -1};
};

class RayCaster : public QObject
{
    Q_OBJECT

public:
    explicit RayCaster(QObject *parent = nullptr);

    /**
     * @brief Returns current ray direction
     * @return current ray direction
     */
    QVector3D direction() const;

    /**
     * @brief Returns current ray length
     * @return Current ray length
     */
    float length() const;

    /**
     * @brief Returns current ray origin
     * @return Current ray origin
     */
    QVector3D origin() const;

    /**
     * @brief Returns list of last ray caster hits
     * @return List of last ray caster hits
     */
    QList <RayCasterHit> hits() const;

public Q_SLOTS:
    /**
     * @brief Appends object for ray cast test
     * @param object - object for ray cast test
     */
    void addObject(std::weak_ptr <SceneObject> object);

    /**
     * @brief Sets ray direction
     * @param[in] direction - ray direction
     */
    void setDirection(const QVector3D& direction);

    /**
     * @brief Sets ray origin
     * @param[in] origin - ray origin
     */
    void setOrigin(const QVector3D& origin);

    /**
     * @brief Sets ray length
     * @param[in] length - ray length
     */
    void setLength(float length);

    /**
     * @brief Triggers ray cast procedure
     */
    void trigger();

    /**
     * @brief Triggers ray cast procedure
     * @param[in] origin - ray origin
     * @param[in] direction - ray direction
     * @param[in] length - ray length
     */
    void trigger(const QVector3D& origin, const QVector3D& direction, float length = 1000.0f);

private:
    void pickAsTriangles(std::shared_ptr <SceneObject> object);
    void pickAsQuads(std::shared_ptr <SceneObject> object);

private:
    QVector3D m_origin;
    QVector3D m_direction;
    float m_length = 1000.0f;
    QList <RayCasterHit> m_hits;
    QList <std::weak_ptr <SceneObject>> m_objects;
};

#endif // RAYCASTER_H
