#include "tinbasicsurfaceprocessor.hpp"

VertexObject TinBasicSurfaceProcessor::process(const QVector <QVector3D>& points)
{
    // Удаляем повторяющиеся точки используя множество
    QSet <Point3D <float>> set;

    for (const auto& _p : points){
        Point3D <float> p(_p.x(), _p.y(), _p.z());
        set.insert(p);
    }

    // Формируем вектор точек для триангуляции
    std::vector <Point3D <double>> input;

    for (const auto& _p : set){
        Point3D <double> p;
        p.setX(static_cast <double> (_p.x()));
        p.setY(static_cast <double> (_p.y()));
        p.setZ(static_cast <double> (_p.z()));

        input.push_back(p);
    }

    // Создаем объект и выполняем триангуляцию
    Delaunay <double> triangulator;

    auto pTriangles = triangulator.trinagulate(input, mTriangleEdgeLengthLimit);

    // Формируем результат в необходимом формате и возвращаем
    VertexObject object(GL_TRIANGLES);

    for (const auto& _t : *pTriangles){

        //qDebug() << "AB -> " << _t.AB().length() << ", BC -> " << _t.BC().length() << ", AC -> " << _t.AC().length();

        if (_t.AB().length() <= mTriangleEdgeLengthLimit &&
            _t.BC().length() <= mTriangleEdgeLengthLimit &&
            _t.AC().length() <= mTriangleEdgeLengthLimit){
            object.append({QVector3D(_t.A().x(), _t.A().y(), _t.A().z()),
                           QVector3D(_t.B().x(), _t.B().y(), _t.B().z()),
                           QVector3D(_t.C().x(), _t.C().y(), _t.C().z())});
        }
    }

    return object;
}

void TinBasicSurfaceProcessor::setParam(Param p, QVariant value)
{
    switch(p){
        case Param::triangleEdgeLengthLimit:
            mTriangleEdgeLengthLimit = value.toDouble();
        break;
    }
}
