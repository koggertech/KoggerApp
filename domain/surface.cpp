#include "surface.h"
#include <boundarydetector.h>
#include <Triangle.h>
#include <drawutils.h>

Surface::Surface(QObject* parent)
: SceneGraphicsObject(parent)
, m_contour(std::make_shared <Contour>())
, m_grid(std::make_shared <SurfaceGrid>())
{
    setPrimitiveType(GL_TRIANGLES);
}

Surface::~Surface()
{}

SceneObject::SceneObjectType Surface::type() const
{
    return SceneObjectType::Surface;
}

void Surface::draw(QOpenGLFunctions* ctx,
                   const QMatrix4x4& mvp,
                   const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const
{

    if(!shaderProgramMap.contains("height"))
        return;

    drawSurface(ctx, mvp, shaderProgramMap["height"].get());

    if(!shaderProgramMap.contains("static"))
        return;

    drawContour(ctx, mvp, shaderProgramMap["static"].get());
    drawGrid(ctx, mvp, shaderProgramMap["static"].get());
}

void Surface::drawSurface(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, QOpenGLShaderProgram *shaderProgram) const
{
    if(!m_isVisible)
        return;

    if (!shaderProgram->bind()){
        qCritical() << "Error binding shader program.";
        return;
    }

    int posLoc    = shaderProgram->attributeLocation("position");
    int maxZLoc   = shaderProgram->uniformLocation("max_z");
    int minZLoc   = shaderProgram->uniformLocation("min_z");
    int matrixLoc = shaderProgram->uniformLocation("matrix");

    shaderProgram->setUniformValue(maxZLoc, m_boundingBox.maximumZ());
    shaderProgram->setUniformValue(minZLoc, m_boundingBox.minimumZ());
    shaderProgram->setUniformValue(matrixLoc, mvp);
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, m_data.constData());

    ctx->glLineWidth(4.0);
    ctx->glDrawArrays(m_primitiveType, 0, m_data.size());
    ctx->glLineWidth(1.0);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}

void Surface::drawContour(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, QOpenGLShaderProgram *shaderProgram) const
{
    if(!m_contour->isVisible())
        return;

    if (!shaderProgram->bind()){
        qCritical() << "Error binding shader program.";
        return;
    }

    int posLoc    = shaderProgram->attributeLocation("position");
    int matrixLoc = shaderProgram->uniformLocation("matrix");
    int colorLoc  = shaderProgram->uniformLocation("color");

    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(m_contour->color()));
    shaderProgram->setUniformValue(matrixLoc, mvp);
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, m_contour->cdata().constData());

    ctx->glLineWidth(4.0);
    ctx->glDrawArrays(m_contour->primitiveType(), 0, m_contour->cdata().size());
    ctx->glLineWidth(1.0);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}

void Surface::drawGrid(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, QOpenGLShaderProgram *shaderProgram) const
{
    if(!m_grid->isVisible())
        return;

    if (!shaderProgram->bind()){
        qCritical() << "Error binding shader program.";
        return;
    }

    int posLoc    = shaderProgram->attributeLocation("position");
    int matrixLoc = shaderProgram->uniformLocation("matrix");
    int colorLoc  = shaderProgram->uniformLocation("color");

    shaderProgram->setUniformValue(colorLoc, DrawUtils::colorToVector4d(m_grid->color()));
    shaderProgram->setUniformValue(matrixLoc, mvp);
    shaderProgram->enableAttributeArray(posLoc);
    shaderProgram->setAttributeArray(posLoc, m_grid->cdata().constData());

    ctx->glLineWidth(4.0);
    ctx->glDrawArrays(m_grid->primitiveType(), 0, m_grid->cdata().size());
    ctx->glLineWidth(1.0);

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();
}

Contour *Surface::contour() const
{
    return m_contour.get();
}

SurfaceGrid *Surface::grid() const
{
    return m_grid.get();
}

void Surface::setData(const QVector<QVector3D> &data)
{
    SceneGraphicsObject::setData(data);

    updateGrid();

    updateContour();
}

void Surface::clearData()
{
    SceneGraphicsObject::clearData();

    m_grid->clearData();
    m_contour->clearData();
}

void Surface::append(const QVector3D &vertex)
{
    SceneGraphicsObject::append(vertex);

    updateGrid();

    updateContour();
}

void Surface::append(const QVector<QVector3D> &other)
{
    SceneGraphicsObject::append(other);

    updateGrid();

    updateContour();
}

void Surface::updateGrid()
{
    m_grid->clearData();

    switch(m_primitiveType){
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
    m_grid->clearData();

    if (m_data.size() < 3)
        return;

    for (int i = 0; i < m_data.size()-3; i+=3){
        QVector3D A = m_data[i];
        QVector3D B = m_data[i+1];
        QVector3D C = m_data[i+2];

        A.setZ(A.z() + 0.03);
        B.setZ(B.z() + 0.03);
        C.setZ(C.z() + 0.03);

        m_grid->append({A, B,
                      B, C,
                      A, C});
    }
}

void Surface::makeQuadGrid()
{
    m_grid->clearData();

    if (m_data.size() < 4)
        return;

    for (int i = 0; i < m_data.size()-4; i+=4){
        QVector3D A = m_data[i];
        QVector3D B = m_data[i+1];
        QVector3D C = m_data[i+2];
        QVector3D D = m_data[i+3];

        A.setZ(A.z() + 0.03);
        B.setZ(B.z() + 0.03);
        C.setZ(C.z() + 0.03);
        D.setZ(D.z() + 0.03);

        m_grid->append({A, B,
                      B, C,
                      C, D,
                      A, D});
    }
}

void Surface::makeContourFromTriangles()
{
    BoundaryDetector <float> boundaryDetector;

    std::vector <::Triangle <float>> temp;

    for(int i = 0; i < m_data.size()-3; i+=3){
        temp.push_back(::Triangle <float>(
                            Point3D <float>(m_data[i].x(),   m_data[i].y(),   m_data[i].z()),
                            Point3D <float>(m_data[i+1].x(), m_data[i+1].y(), m_data[i+1].z()),
                            Point3D <float>(m_data[i+2].x(), m_data[i+2].y(), m_data[i+2].z())
                        ));
    }

    auto boundary = boundaryDetector.detect(temp);

    for(const auto& segment : boundary){
        m_contour->append({segment.p1().toQVector3D(),
                          segment.p2().toQVector3D()
                         });
    };
}

void Surface::makeContourFromQuads()
{
    BoundaryDetector <float> boundaryDetector;

    std::vector <::Quad <float>> temp;

    for(int i = 0; i < m_data.size()-4; i+=4){
        temp.push_back(::Quad <float>(
                            Point3D <float>(m_data[i].x(),   m_data[i].y(),   m_data[i].z()),
                            Point3D <float>(m_data[i+1].x(), m_data[i+1].y(), m_data[i+1].z()),
                            Point3D <float>(m_data[i+2].x(), m_data[i+2].y(), m_data[i+2].z()),
                            Point3D <float>(m_data[i+3].x(), m_data[i+3].y(), m_data[i+3].z())
                        ));
    }

    auto boundary = boundaryDetector.detect(temp);

    for(const auto& segment : boundary){
        m_contour->append({segment.p1().toQVector3D(),
                          segment.p2().toQVector3D()
                         });
    };
}

void Surface::updateContour()
{
    m_contour->clearData();

    switch(m_primitiveType){
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
