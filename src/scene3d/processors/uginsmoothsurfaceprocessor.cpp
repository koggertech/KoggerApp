#include "uginsmoothsurfaceprocessor.hpp"

VertexObject UginSmoothSurfaceProcessor::process(const QVector <QVector3D>& points)
{
    std::vector <Triangle <float>> triangles;

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

    auto comparator = [](const Triangle <float>& t1, const Triangle <float>& t2)
    {
        return t1.maxLengthEdge() < t2.maxLengthEdge();
    };

    std::sort(triangles.begin(), triangles.end(), comparator);

    for (const auto& t : triangles){
        qDebug() << "AB -> " << t.AB().length() << ", BC -> " << t.BC().length() << ", AC -> " << t.AC().length();
    }

    Interpolator <float> ugin;

    ugin.setInterpolationLevel(mInterpolationLevel);

    auto quads = ugin.interpolate(&triangles);

    VertexObject object(GL_QUADS);

    for (const auto& q : *quads){
        QVector3D A(q.A().x(), q.A().y(), q.A().z());
        QVector3D B(q.B().x(), q.B().y(), q.B().z());
        QVector3D C(q.C().x(), q.C().y(), q.C().z());
        QVector3D D(q.D().x(), q.D().y(), q.D().z());

        object.append({A,B,C,D});
    }

    return object;
}

void UginSmoothSurfaceProcessor::setParam(Param p, QVariant value)
{
    switch(p){
        case Param::interpolationLevel:
            mInterpolationLevel = value.toInt();
        break;
    default:
        break;
    }
}
