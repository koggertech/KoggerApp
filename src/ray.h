#ifndef RAY_H
#define RAY_H

#include <QObject>
#include <QVector3D>

#include <memory>

class SceneObject;
class RayHit
{
public:
    std::weak_ptr <SceneObject> object() const;
    QVector3D worldIntersection() const;
    QPair<int, int> indices() const;
    void setObject(std::weak_ptr <SceneObject> object);
    void setWorldIntersection(const QVector3D& intersection);
    void setIndices(int begin, int end);

private:
    std::weak_ptr <SceneObject> m_object;
    QVector3D m_worldIntersection;
    QPair <int, int> m_indices = {-1, -1};
};

class Ray : public QObject
{
    Q_OBJECT
public:
    enum class HittingMode
    {
        Vertex = 0,
        Segment = 1,
        Triangle = 2,
        Quad = 3
    };

    explicit Ray(QObject *parent = nullptr);
    explicit Ray(const QVector3D& origin,
                 const QVector3D& direction,
                 QObject *parent = nullptr);

public:
    QVector3D origin() const;
    QVector3D direction() const;
    QVector<RayHit> hitObject(std::weak_ptr<SceneObject> object, HittingMode hittingMode);

private:
    QVector<RayHit> pickAsVertex(std::weak_ptr<SceneObject> object);
    QVector<RayHit> pickAsTriangles(std::weak_ptr<SceneObject> object);
    QVector<RayHit> pickAsQuads(std::weak_ptr<SceneObject> object);
    QVector<RayHit> pickAsSegments(std::weak_ptr<SceneObject> object);

public Q_SLOTS:
    void setOrigin(const QVector3D& origin);
    void setDirection(const QVector3D& dir);

Q_SIGNALS:
    void originChanged(QVector3D origin);
    void directionChanged(QVector3D direction);

protected:
    QVector3D m_origin={0.0f, 0.0f, 0.0f};
    QVector3D m_direction={0.0f,0.0f,-1.0f};
};

#endif // RAY_H
