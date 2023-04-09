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

    if(params.gridType() == GRID_TYPE_QUAD){

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


    }else if (params.gridType() == GRID_TYPE_TRIANGLE){

        qDebug() << "Generating triangle grid...";

        auto triangles = GridGenerator <double>::generateTriangleGrid(topLeft, gridW, gridH, params.gridCellSideSize());

        qDebug() << "Filtering triangle grid...";

        auto cell = triangles->begin();
        while (cell != triangles->end()){
            auto triangleCell = *cell;

            bool surfaceContainsA = false;
            bool surfaceContainsB = false;
            bool surfaceContainsC = false;


            auto t = pTriangles->begin();
            while (t != pTriangles->end() && (!surfaceContainsA || !surfaceContainsB || !surfaceContainsC))
            {
                if (!surfaceContainsA) surfaceContainsA = t->contains(triangleCell.A());
                if (!surfaceContainsB) surfaceContainsB = t->contains(triangleCell.B());
                if (!surfaceContainsC) surfaceContainsC = t->contains(triangleCell.C());
                t++;
            }

            if (t != pTriangles->end()){
                grid.push_back(triangleCell.A());
                grid.push_back(triangleCell.B());
                grid.push_back(triangleCell.C());
            }

            //object.append(QVector3D(triangleCell.A().x(), triangleCell.A().y(), triangleCell.A().z()));
            //object.append(QVector3D(triangleCell.B().x(), triangleCell.B().y(), triangleCell.B().z()));
            //object.append(QVector3D(triangleCell.C().x(), triangleCell.C().y(), triangleCell.C().z()));

            cell++;
        }
    }

    // Интерполируем z - координату сетки
    BarycentricInterpolator <double> interpolator;

    auto triangles = *pTriangles;

    interpolator.process(triangles, grid);

    // Формируем вершинный объект

    for (const auto& p : grid)
        object.append(QVector3D(p.x(), p.y(), p.z()));

    return object;
}
