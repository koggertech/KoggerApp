#include "markupgrid.h"

MarkupGrid::MarkupGrid(QObject* parent)
    : DisplayedObject(GL_LINES, parent)
{

}

void MarkupGrid::setSize(QVector3D topLeft, float width, float height, float dist)
{
    Point3D <float> point{
                topLeft.x(),
                topLeft.y(),
                topLeft.z()
                };

    auto grid = GridGenerator <float>::generateQuadGrid(point,width, height, dist);

    QVector <QVector3D> staged;
    QSet <QPair <Point3D <float>,Point3D <float>>> set;

    for (const auto& q : *grid){
        set.insert({q.A(),q.B()});
        set.insert({q.B(),q.C()});
        set.insert({q.C(),q.D()});
        set.insert({q.D(),q.A()});
    }

    auto it = set.begin();
    while (it != set.end()){
        staged.append({
                        it->first.toQVector3D(),
                        it->second.toQVector3D()
                      });
        it++;
    }

    VertexObject::setData(staged);
}

void MarkupGrid::setSize(QVector3D topLeft, QVector3D bottomRight, float dist)
{
    //auto result = std::make_shared <std::vector <Quad <T>>>();

    //int horzCellsCount = std::round(std::abs(topLeft.x() - bottomRight.x()) / dist) + 1;
    //int vertCellsCount = std::round(std::abs(topLeft.y() - bottomRight.y() / dist)+ 1;

    //for (int row = 0; row < horzCellsCount; row++){
    //    for (int col = 0; col < vertCellsCount; col++){
    //        Point3D <T> A(topLeft.x() + col * cellSideSize,
    //                      topLeft.y() + row * cellSideSize,
    //                      topLeft.z());

    //        Point3D <T> B(topLeft.x() + (col+1) * cellSideSize,
    //                      topLeft.y() + row * cellSideSize,
    //                      topLeft.z());

    //        Point3D <T> C(topLeft.x() + (col+1) * cellSideSize,
    //                      topLeft.y() + (row+1) * cellSideSize,
    //                      topLeft.z());

    //        Point3D <T> D(topLeft.x() + (col) * cellSideSize,
    //                      topLeft.y() + (row+1) * cellSideSize,
    //                      topLeft.z());

    //        Quad <T> quad(A,B,C,D);

    //        result->push_back(quad);
    //    }
    //}

}
