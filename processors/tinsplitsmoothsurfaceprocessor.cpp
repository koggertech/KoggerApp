#include "tinsplitsmoothsurfaceprocessor.hpp"

void TinSplitSmoothSurfaceProcessor::calculateZ(Triangle <float>& t, Point3D <float>& p)
{
    auto& c = p;
    float w_1 = ((t.A().y() - t.C().y()) * (c.x() - t.C().x()) + (t.C().x() - t.A().x()) * (c.y() - t.C().y())) /
                 ((t.A().y() - t.C().y()) * (t.B().x() - t.C().x()) + (t.C().x() - t.A().x())*(t.B().y() - t.C().y()));

    float w_2 = ((t.C().y() - t.B().y()) * (c.x() - t.C().x()) + (t.B().x() - t.C().x()) * (c.y() - t.C().y())) /
                 ((t.A().y() - t.C().y()) * (t.B().x() - t.C().x()) + (t.C().x() - t.A().x())*(t.B().y() - t.C().y()));

    float w_3 = 1.0f - w_1 - w_2;

    float z = w_1 * t.B().z() + w_2 * t.C().z() + w_3 * t.A().z();

    p.setZ(z);
}

VertexObject TinSplitSmoothSurfaceProcessor::process(const QVector<QVector3D> &points)
{
    Point3D <float> topLeft(0.0f,0.0f,0.0f);
    int width = 10;
    int height = 20;
    float cellSideSize = 0.2f;

    auto grid = GridGenerator <float>::generateQuadGrid(topLeft, width, height, cellSideSize);


    std::vector <Triangle <float>> triangles;

    {
        auto it = points.begin();
        while (it != points.end()){
            Point3D <float> A, B, C;

            A.setX(it->x());
            A.setY(it->y());
            A.setZ(it->z());

            it++;

            B.setX(it->x());
            B.setY(it->y());
            B.setZ(it->z());

            it++;

            C.setX(it->x());
            C.setY(it->y());
            C.setZ(it->z());

            it++;

            Triangle <float> triangle(A,B,C);

            triangles.push_back(triangle);
        }
    }

    //std::vector <Triangle <double>> grid;
    //grid.push_back(triangles.front());


    std::vector <Triangle <float>> result;

    auto it = triangles.begin();
    while (it != triangles.end()){

        std::queue <Triangle <float>> queue;

        queue.push(*it);

        while (!queue.empty()){

            auto t = queue.front();

            //qDebug() << "AB -> " << t.AB().length() << ", BC -> " << t.BC().length() << ", AC -> " << t.AC().length();

            if(t.AB().length() > mTriangleEdgeMinLength ||
               t.BC().length() > mTriangleEdgeMinLength ||
               t.AC().length() > mTriangleEdgeMinLength)
            {
                auto splitted = t.splitByCenter();

                for (const auto& _t : splitted){
                    queue.push(_t);
                    //qDebug() << "After split: " << "AB -> " << _t.AB().length() << ", BC -> " << _t.BC().length() << ", AC -> " << _t.AC().length();
                }
            }else{
                result.push_back(t);
            }

            queue.pop();
        }

        it++;
    }

    qDebug() << "Grid size after split -> " << triangles.size();

    // Этап 1 - интерполируем координату Z

    VertexObject vertexObject(GL_TRIANGLES);

    for (const auto& tg : result){

        auto A = tg.A();
        auto B = tg.B();
        auto C = tg.C();

        auto it_1 = triangles.begin();
        while (it_1 != triangles.end() && !it_1->contains(A)){
            it_1++;
        }


        auto it_2 = triangles.begin();
        while (it_2 != triangles.end() && !it_2->contains(B)){
            it_2++;
        }



        auto it_3 = triangles.begin();
        while (it_3 != triangles.end() && !it_3->contains(C)){
            it_3++;
        }

        if (it_1 != triangles.end() && it_2 != triangles.end() && it_3 != triangles.end()){
            calculateZ(*it_1, A);
            calculateZ(*it_2, B);
            calculateZ(*it_3, C);
        }

        Triangle <float> tri(A,B,C);

        vertexObject.append({QVector3D(tri.A().x(), tri.A().y(), tri.A().z()),
                             QVector3D(tri.B().x(), tri.B().y(), tri.B().z()),
                             QVector3D(tri.C().x(), tri.C().y(), tri.C().z())});
    }


    return vertexObject;
}



void TinSplitSmoothSurfaceProcessor::setParam(Param p, QVariant value)
{
    switch(p){
        case Param::triangleEdgeMinLength:
            mTriangleEdgeMinLength = value.toFloat();
        break;
    default:
        break;
    }
}
