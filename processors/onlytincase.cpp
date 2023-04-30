#include "onlytincase.h"

OnlyTinCase::OnlyTinCase()
{

}

VertexObject OnlyTinCase::process(const QVector<QVector3D> &bottomTrack, SceneParams params)
{
    std::set <Point2D <double>> set;
    std::vector <Point3D <double>> input;

    for (size_t i = 0; i < bottomTrack.size(); i++){
        Point2D <double> point(static_cast <double> (bottomTrack.at(i).x()),
                               static_cast <double> (bottomTrack.at(i).y()),
                               static_cast <double> (i));

        set.insert(point);
    }

    for (const auto& p : set){
        Point3D <double> point(p.x(),
                               p.y(),
                               static_cast <double> (bottomTrack.at(p.index()).z()),
                               p.index());
        input.push_back(point);
    }

    Delaunay <double> delaunay;
    auto triangles = delaunay.trinagulate(input, params.triangulationEdgeLengthLimit());

    //DelaunayTin::BowyerWatson <double> delaunay;
    //auto triangles = delaunay.build(input, params.triangulationEdgeLengthLimit());

    BoundaryDetector <double> boundaryDetector;
    auto boundary = boundaryDetector.detect(*triangles);

    //auto boundary = BoundaryDetector <double>::simpleTinBoundary(delaunayResult);

    mContourVertexObject.setPrimitiveType(GL_LINES);

    for (const auto& edge : boundary){
        mContourVertexObject.append({QVector3D(edge.p1().x(), edge.p1().y(), edge.p1().z()),
                                     QVector3D(edge.p2().x(), edge.p2().y(), edge.p2().z())});
    }

    // Формируем вершинный объект
    VertexObject object(GL_TRIANGLES);
    for (const auto& t : *triangles){
        object.append(QVector3D(t.A().x(), t.A().y(), t.A().z()));
        object.append(QVector3D(t.B().x(), t.B().y(), t.B().z()));
        object.append(QVector3D(t.C().x(), t.C().y(), t.C().z()));
    }

    return object;
}
