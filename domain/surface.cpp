#include "surface.h"
#include <boundarydetector.h>

Surface::Surface(QObject* parent)
: DisplayedObject(parent)
, mContour(std::make_shared <Contour>())
, mGrid(std::make_shared <SurfaceGrid>())
{

}

Surface::~Surface()
{

}

Contour *Surface::contour() const
{
    return mContour.get();
}

SurfaceGrid *Surface::grid() const
{
    return mGrid.get();
}

void Surface::setData(const QVector<QVector3D> &data)
{
    VertexObject::setData(data);

    updateGrid();

    updateContour();
}

void Surface::append(const QVector3D &vertex)
{
    VertexObject::append(vertex);

    updateGrid();

    updateContour();
}

void Surface::append(const QVector<QVector3D> &other)
{
    VertexObject::append(other);

    updateGrid();

    updateContour();
}

QString Surface::bottomTrackId() const
{
    return mBottomTrackId;
}

void Surface::setBottomTrackId(QString id)
{
    if(mBottomTrackId == id)
        return;

    mBottomTrackId = id;

    Q_EMIT bottomTrackIdChanged(mBottomTrackId);
}

void Surface::updateGrid()
{
    mGrid->clearData();

    switch(mPrimitiveType){
    case GL_TRIANGLES:
        makeTriangleGrid();
        break;
    case GL_QUADS:
        makeQuadGrid();
        break;
    default:
        return;
    }
}

void Surface::makeTriangleGrid()
{
    mGrid->clearData();

    if (mData.size() < 3)
        return;

    for (int i = 0; i < mData.size()-3; i+=3){
        QVector3D A = mData[i];
        QVector3D B = mData[i+1];
        QVector3D C = mData[i+2];

        A.setZ(A.z() + 0.03);
        B.setZ(B.z() + 0.03);
        C.setZ(C.z() + 0.03);

        mGrid->append({A, B,
                      B, C,
                      A, C});
    }
}

void Surface::makeQuadGrid()
{
    mGrid->clearData();

    if (mData.size() < 4)
        return;

    for (int i = 0; i < mData.size()-4; i+=4){
        QVector3D A = mData[i];
        QVector3D B = mData[i+1];
        QVector3D C = mData[i+2];
        QVector3D D = mData[i+3];

        A.setZ(A.z() + 0.03);
        B.setZ(B.z() + 0.03);
        C.setZ(C.z() + 0.03);
        D.setZ(D.z() + 0.03);

        mGrid->append({A, B,
                      B, C,
                      C, D,
                      A, D});
    }
}

void Surface::makeContourFromTriangles()
{
    BoundaryDetector <float> boundaryDetector;

    std::vector <Triangle <float>> temp;

    for(int i = 0; i < mData.size()-3; i+=3){
        temp.push_back(Triangle <float>(
                            Point3D <float>(mData[i].x(),   mData[i].y(),   mData[i].z()),
                            Point3D <float>(mData[i+1].x(), mData[i+1].y(), mData[i+1].z()),
                            Point3D <float>(mData[i+2].x(), mData[i+2].y(), mData[i+2].z())
                        ));
    }

    auto boundary = boundaryDetector.detect(temp);

    for(const auto& segment : boundary){
        mContour->append({segment.p1().toQVector3D(),
                          segment.p2().toQVector3D()
                         });
    };
}

void Surface::makeContourFromQuads()
{
    BoundaryDetector <float> boundaryDetector;

    std::vector <Quad <float>> temp;

    for(int i = 0; i < mData.size()-4; i+=4){
        temp.push_back(Quad <float>(
                            Point3D <float>(mData[i].x(),   mData[i].y(),   mData[i].z()),
                            Point3D <float>(mData[i+1].x(), mData[i+1].y(), mData[i+1].z()),
                            Point3D <float>(mData[i+2].x(), mData[i+2].y(), mData[i+2].z()),
                            Point3D <float>(mData[i+3].x(), mData[i+3].y(), mData[i+3].z())
                        ));
    }

    auto boundary = boundaryDetector.detect(temp);

    for(const auto& segment : boundary){
        mContour->append({segment.p1().toQVector3D(),
                          segment.p2().toQVector3D()
                         });
    };
}

void Surface::updateContour()
{
    mContour->clearData();

    switch(mPrimitiveType){
    case GL_TRIANGLES:
        makeContourFromTriangles();
        break;
    case GL_QUADS:
        makeContourFromQuads();
        break;
    default:
        return;
    }
}
