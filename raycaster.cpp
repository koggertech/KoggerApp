#include "raycaster.h"

#include <QVector3D>
#include <GL/gl.h>

#include <sceneobject.h>
#include <raycastpickerfactory.h>

RayCaster::RayCaster(QObject *parent)
    : QObject{parent}
{}

QVector3D RayCaster::direction() const
{
    return m_direction;
}

float RayCaster::length() const
{
    return m_length;
}

QVector3D RayCaster::origin() const
{
    return m_origin;
}

QList<RayCasterHit> RayCaster::hits() const
{
    return m_hits;
}

void RayCaster::addObject(std::weak_ptr<SceneObject> object)
{
    m_objects.append(object);
}

void RayCaster::setDirection(const QVector3D &direction)
{
    if(m_direction != direction)
        m_direction = direction;
}

void RayCaster::setOrigin(const QVector3D &origin)
{
    if(m_origin != origin)
        m_origin = origin;
}

void RayCaster::setLength(float length)
{
    if(m_length != length)
        m_length = length;
}

void RayCaster::trigger()
{
    m_hits.clear();

    for(const auto& object : qAsConst(m_objects)){
        auto _object = object.lock();

        if(!_object)
            continue;

        switch(_object->primitiveType()){
        case GL_TRIANGLES:
            pickAsTriangles(_object);
            break;
        case GL_QUADS:
            pickAsQuads(_object);
            break;
        }
    }
}

void RayCaster::trigger(const QVector3D &origin, const QVector3D &direction, float length)
{
    if(m_origin != origin)
        m_origin = origin;

    if(m_direction != direction)
        m_direction = direction;

    if(m_length != length)
        m_length = length;

    trigger();
}

void RayCaster::pickAsTriangles(std::shared_ptr<SceneObject> object)
{
    auto size = object->cdata().size();

    for (int i = 0; i < size; i+=3){

        Triangle <float> triangle(
                                  object->cdata().at(i),
                                  object->cdata().at(i+1),
                                  object->cdata().at(i+2)
                                );

        QVector3D intersectionPoint;

        bool intersects = triangle.intersectsWithLine(m_origin, m_direction, intersectionPoint, true);

        if (intersects){
            RayCasterHit hit;
            hit.setIndices(i, i+3);
            hit.setWorldIntersection(intersectionPoint);
            hit.setSourceObject(object);
            hit.setSourcePrimitive({
                                {   triangle.A().toQVector3D(),
                                    triangle.B().toQVector3D(),
                                    triangle.C().toQVector3D()
                                },
                                GL_TRIANGLES
                                });


            m_hits.append(hit);
        }
    }
}

void RayCaster::pickAsQuads(std::shared_ptr<SceneObject> object)
{
    auto size = object->cdata().size();

    for (int i = 0; i < size; i+=4){

        Quad <float> quad(
                           object->cdata().at(i),
                           object->cdata().at(i+1),
                           object->cdata().at(i+2),
                           object->cdata().at(i+3)
                        );

        QVector3D intersectionPoint;

        bool intersects = quad.intersectsWithLine(m_origin, m_direction, intersectionPoint, true);

        if (intersects){
            RayCasterHit hit;
            hit.setIndices(i, i+4);
            hit.setWorldIntersection(intersectionPoint);
            hit.setSourceObject(object);
            hit.setSourcePrimitive({
                                {   quad.A().toQVector3D(),
                                    quad.B().toQVector3D(),
                                    quad.C().toQVector3D(),
                                    quad.D().toQVector3D()
                                },
                                GL_QUADS
                                });
            m_hits.append(hit);
        }
    }
}

std::weak_ptr<SceneObject> RayCasterHit::sourceObject() const
{
    return m_sourceObject;
}

std::pair<QVector<QVector3D>, int> RayCasterHit::sourcePrimitive() const
{
    return m_sourcePrimitive;
}

QVector3D RayCasterHit::worldIntersection() const
{
    return m_worldIntersection;
}

QPair<int, int> RayCasterHit::indices() const
{
    return m_indices;
}

void RayCasterHit::setSourceObject(std::weak_ptr<SceneObject> object)
{
    m_sourceObject = object;
}

void RayCasterHit::setSourcePrimitive(std::pair<QVector<QVector3D>, int> primitive)
{
    if(m_sourcePrimitive != primitive)
        m_sourcePrimitive = primitive;
}

void RayCasterHit::setWorldIntersection(const QVector3D &intersection)
{
    if(m_worldIntersection != intersection)
        m_worldIntersection = intersection;
}

void RayCasterHit::setIndices(int begin, int end)
{
    m_indices = {begin, end};
}
