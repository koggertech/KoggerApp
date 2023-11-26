#include "tinwithbarycentriccase.h"

TinWithBarycentricCase::TinWithBarycentricCase()
{

}

VertexObject TinWithBarycentricCase::process(const QVector <QVector3D>& bottomTrack, SceneParams params)
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

    // Находим верхний левый угол триангулированной поверхности
    double min_x = pTriangles->front().A().x();
    double min_y = pTriangles->front().A().y();
    double max_x = min_x;
    double max_y = min_y;

    for (const auto& t : *pTriangles){
        auto local_min_x = std::min(t.A().x(), t.B().x());
        local_min_x = std::min(local_min_x, t.C().x());

        auto local_min_y = std::min(t.A().y(), t.B().y());
        local_min_y = std::min(local_min_y, t.C().y());

        auto local_max_x = std::max(t.A().x(), t.B().x());
        local_max_x = std::max(local_max_x, t.C().x());

        auto local_max_y = std::max(t.A().y(), t.B().y());
        local_max_y = std::max(local_max_y, t.C().y());

        min_x = std::min(min_x, local_min_x);
        min_y = std::min(min_y, local_min_y);
        max_x = std::max(max_x, local_max_x);
        max_y = std::max(max_y, local_max_y);
    }

    // Формируем сетку для интерполяции

    Point3D <double> topLeft(min_x, min_y, 0.0f);

    double gridW = max_y - min_y;
    double gridH = max_x - min_x;

    VertexObject object(GL_TRIANGLES);

    std::vector <Point3D <double>> grid;

    object.setPrimitiveType(GL_QUADS);

    auto quads = GridGenerator <double>::generateQuadGrid(topLeft, gridW, gridH, params.gridCellSideSize());

    auto q = quads->begin();
    while (q != quads->end()){
        auto quad = *q;

        bool surfaceContainsA = false;
        bool surfaceContainsB = false;
        bool surfaceContainsC = false;
        bool surfaceContainsD = false;

        auto t = pTriangles->begin();
        while (t != pTriangles->end() && (!surfaceContainsA || !surfaceContainsB || !surfaceContainsC || !surfaceContainsD))
        {
            if (!surfaceContainsA) surfaceContainsA = t->contains(quad.A());
            if (!surfaceContainsB) surfaceContainsB = t->contains(quad.B());
            if (!surfaceContainsC) surfaceContainsC = t->contains(quad.C());
            if (!surfaceContainsD) surfaceContainsD = t->contains(quad.D());
            t++;
        }

        if (t != pTriangles->end()){
            grid.push_back(quad.A());
            grid.push_back(quad.B());
            grid.push_back(quad.C());
            grid.push_back(quad.D());
        }
        q++;
    }

    // Интерполируем z - координату сетки
    BarycentricInterpolator <double> interpolator;

    auto triangles = *pTriangles;

    interpolator.process(triangles, grid);

    std::vector <Quad <double>> interpolatedQuads;
    for(size_t i = 0; i < grid.size()-4; i+=4){
        Quad <double> quad(grid[i],grid[i+1],grid[i+2],grid[i+3]);
        interpolatedQuads.push_back(quad);
    }

    BoundaryDetector <double> boundaryDetector;
    auto boundary = boundaryDetector.detect(interpolatedQuads);

    //auto boundary = BoundaryDetector <double>::uniformGridBoundary(grid);

    // Формируем вершинный объект

    for (const auto& p : grid)
        object.append(QVector3D(p.x(), p.y(), p.z()));

    mContourVertexObject.setPrimitiveType(GL_LINES);

    for (const auto& edge : boundary){
        mContourVertexObject.append({QVector3D(edge.p1().x(), edge.p1().y(), edge.p1().z()),
                                    QVector3D(edge.p2().x(), edge.p2().y(), edge.p2().z())});
    }

    return object;
}
