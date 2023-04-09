#include "onlytincase.h"

OnlyTinCase::OnlyTinCase()
{

}

VertexObject OnlyTinCase::process(const QVector<QVector3D> &bottomTrack, SceneParams params)
{
    // Удаляем повторяющиеся точки используя множество
    QSet <Point3D <double>> set;

    for (const auto& _p : bottomTrack){
        Point3D <double> p(_p.x(), _p.y(), _p.z());
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

    // Выполняем триангуляцию
    Delaunay <double> triangulator;

    auto pTriangles = triangulator.trinagulate(input, params.triangulationEdgeLengthLimit());

    // Формируем вершинный объект
    VertexObject object(GL_TRIANGLES);
    for (const auto& t : *pTriangles){
        object.append(QVector3D(t.A().x(), t.A().y(), t.A().z()));
        object.append(QVector3D(t.B().x(), t.B().y(), t.B().z()));
        object.append(QVector3D(t.C().x(), t.C().y(), t.C().z()));
    }

    return object;
}
