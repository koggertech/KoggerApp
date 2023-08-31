#include "surfaceprocessor.h"

#include <set>

#include <Point3D.h>
#include <DelaunayTriangulation.h>
#include <gridgenerator.h>
#include <barycentricinterpolator.h>

SurfaceProcessor::SurfaceProcessor(QObject *parent)
    : QObject{parent}
{

}

void SurfaceProcessor::process(QVector <QVector3D> sourceData,
                               Surface* surface,
                               bool needSmoothing,
                               float cellSize)
{
    Q_EMIT processingStarted();

    std::set <Point2D <double>> set;
    std::vector <Point3D <double>> input;

    for (size_t i = 0; i < sourceData.size(); i++){
        Point2D <double> point(static_cast <double> (sourceData.at(i).x()),
                               static_cast <double> (sourceData.at(i).y()),
                               static_cast <double> (i));

        set.insert(point);
    }

    for (const auto& p : set){
        Point3D <double> point(p.x(),
                               p.y(),
                               static_cast <double> (sourceData.at(p.index()).z()),
                               p.index());
        input.push_back(point);
    }

    Delaunay <double> delaunay;
    auto triangles = delaunay.trinagulate(input);

    surface->clearData();

    if(needSmoothing){
        surface->setPrimitiveType(GL_QUADS);

        std::vector <Point3D <double>> trimmedGrid;

        auto fullGrid = GridGenerator <double>::generateQuadGrid(Point3D <double>(
                                                                surface->bounds().minimumX(),
                                                                surface->bounds().minimumY(),
                                                                surface->bounds().minimumZ()
                                                            ),
                                                            surface->bounds().width(),
                                                            surface->bounds().length(),
                                                            cellSize);

        auto q = fullGrid->begin();
        while (q != fullGrid->end()){
            auto quad = *q;

            bool surfaceContainsA = false;
            bool surfaceContainsB = false;
            bool surfaceContainsC = false;
            bool surfaceContainsD = false;

            auto t = triangles->begin();
            while (t != triangles->end() && (!surfaceContainsA ||
                                             !surfaceContainsB ||
                                             !surfaceContainsC ||
                                             !surfaceContainsD))
            {
                if (!surfaceContainsA) surfaceContainsA = t->contains(quad.A());
                if (!surfaceContainsB) surfaceContainsB = t->contains(quad.B());
                if (!surfaceContainsC) surfaceContainsC = t->contains(quad.C());
                if (!surfaceContainsD) surfaceContainsD = t->contains(quad.D());
                t++;
            }

            if (t != triangles->end()){
                trimmedGrid.push_back(quad.A());
                trimmedGrid.push_back(quad.B());
                trimmedGrid.push_back(quad.C());
                trimmedGrid.push_back(quad.D());
            }
            q++;
        }

        BarycentricInterpolator <double> interpolator;

        auto trianglesTemp = *triangles;

        interpolator.process(trianglesTemp, trimmedGrid);

        for(const auto& point : trimmedGrid){
          surface->append(point.toQVector3D());
        }

        //for(const auto& quad : *fullGrid){
        //    surface->append(quad.A().toQVector3D());
        //    surface->append(quad.B().toQVector3D());
        //    surface->append(quad.C().toQVector3D());
        //    surface->append(quad.D().toQVector3D());
        //}

    }else {
        surface->setPrimitiveType(GL_TRIANGLES);

        for (const auto& t : *triangles){
           surface->append(QVector3D(t.A().x(), t.A().y(), t.A().z()));
           surface->append(QVector3D(t.B().x(), t.B().y(), t.B().z()));
           surface->append(QVector3D(t.C().x(), t.C().y(), t.C().z()));
        }
    }

    Q_EMIT processingFinished();
}
