#include "surface.h"
#include <boundarydetector.h>
#include <Triangle.h>

Surface::Surface(int primitiveType, QObject* parent)
: DisplayedObject(primitiveType, parent)
, mContour(std::make_shared <Contour>())
, mGrid(std::make_shared <SurfaceGrid>())
{}

Surface::~Surface()
{}

SceneObject::SceneObjectType Surface::type() const
{
    return SceneObjectType::Surface;
}

void Surface::draw(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, QMap <QString, QOpenGLShaderProgram*> shaderProgramMap)
{

    if(!shaderProgramMap.contains("height"))
        return;

    drawSurface(ctx, mvp, shaderProgramMap["height"]);

    if(!shaderProgramMap.contains("static"))
        return;

    drawContour(ctx, mvp, shaderProgramMap["static"]);
    drawGrid(ctx, mvp, shaderProgramMap["static"]);
}

void Surface::drawSurface(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, QOpenGLShaderProgram *shaderProgram)
{
    if(!mIsVisible)
        return;

    if (!shaderProgram->bind()){
        qCritical() << "Error binding shader program.";
        return;
    }

    int posLoc    = shaderProgram->attributeLocation("position");
    int maxZLoc   = shaderProgram->uniformLocation("max_z");
    int minZLoc   = shaderProgram->uniformLocation("min_z");
    int matrixLoc = shaderProgram->uniformLocation("matrix");

    shaderProgram->setUniformValue(maxZLoc, mBounds.maximumZ());
    shaderProgram->setUniformValue(minZLoc, mBounds.minimumZ());
    shaderProgram->setUniformValue(matrixLoc, mvp);
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, mData.constData());

    ctx->glLineWidth(4.0);
    ctx->glDrawArrays(mPrimitiveType, 0, mData.size());
    ctx->glLineWidth(1.0);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}

void Surface::drawContour(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, QOpenGLShaderProgram *shaderProgram)
{
    if(!mContour->isVisible())
        return;

    if (!shaderProgram->bind()){
        qCritical() << "Error binding shader program.";
        return;
    }

    int posLoc    = shaderProgram->attributeLocation("position");
    int matrixLoc = shaderProgram->uniformLocation("matrix");
    int colorLoc  = shaderProgram->uniformLocation("color");

    shaderProgram->setUniformValue(colorLoc, mContour->color4d());
    shaderProgram->setUniformValue(matrixLoc, mvp);
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, mContour->cdata().constData());

    ctx->glLineWidth(4.0);
    ctx->glDrawArrays(mContour->primitiveType(), 0, mContour->cdata().size());
    ctx->glLineWidth(1.0);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}

void Surface::drawGrid(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, QOpenGLShaderProgram *shaderProgram)
{
    if(!mGrid->isVisible())
        return;

    if (!shaderProgram->bind()){
        qCritical() << "Error binding shader program.";
        return;
    }

    int posLoc    = shaderProgram->attributeLocation("position");
    int matrixLoc = shaderProgram->uniformLocation("matrix");
    int colorLoc  = shaderProgram->uniformLocation("color");

    shaderProgram->setUniformValue(colorLoc, mGrid->color4d());
    shaderProgram->setUniformValue(matrixLoc, mvp);
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, mGrid->cdata().constData());

    ctx->glLineWidth(4.0);
    ctx->glDrawArrays(mGrid->primitiveType(), 0, mGrid->cdata().size());
    ctx->glLineWidth(1.0);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
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

    std::vector <::Triangle <float>> temp;

    for(int i = 0; i < mData.size()-3; i+=3){
        temp.push_back(::Triangle <float>(
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

    std::vector <::Quad <float>> temp;

    for(int i = 0; i < mData.size()-4; i+=4){
        temp.push_back(::Quad <float>(
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
