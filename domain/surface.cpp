#include "surface.h"
#include <boundarydetector.h>
#include <Triangle.h>
#include <draw_utils.h>

Surface::Surface(QObject* parent)
: SceneObject(new SurfaceRenderImplementation, parent)
, m_contour(std::make_shared <Contour>())
, m_grid(std::make_shared <SurfaceGrid>())
{
    QObject::connect(m_grid.get(), &SurfaceGrid::changed, [this](){
        RENDER_IMPL(Surface)->m_gridRenderImpl = *m_grid->m_renderImpl;
        Q_EMIT changed();
    });

    QObject::connect(m_contour.get(), &Contour::changed, [this](){
        RENDER_IMPL(Surface)->m_contourRenderImpl = *m_contour->m_renderImpl;
        Q_EMIT changed();
    });
}

Surface::~Surface()
{}

void Surface::setProcessingTask(const SurfaceProcessorTask& task)
{
    m_processingTask = task;
}

SceneObject::SceneObjectType Surface::type() const
{
    return SceneObjectType::Surface;
}

Contour *Surface::contour() const
{
    return m_contour.get();
}

SurfaceGrid *Surface::grid() const
{
    return m_grid.get();
}

SurfaceProcessorTask Surface::processingTask() const
{
    return m_processingTask;
}

void Surface::setLlaRef(LLARef llaRef)
{
    llaRef_ = llaRef;
}

void Surface::saveVerticesToFile(const QString& path)
{
    if (!llaRef_.isInit) {
        qWarning() << "Surface::saveVerticesToFile: !llaRef_.isInit";
        return;
    }

#ifdef Q_OS_ANDROID
    QString filePath = path;
#else
    QString filePath = QUrl(path).toLocalFile();
#endif

    auto* r = RENDER_IMPL(Surface);
    QSet<QVector3D> uniqueVertices(r->m_data.begin(), r->m_data.end());

    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return;
    }

    QTextStream out(&file);
    out << "lat,lon,alt,x,y,z\n";
    for (const QVector3D& vertex : uniqueVertices) {
        NED tmpNed(vertex.x(), vertex.y(), vertex.z());
        LLA lla(&tmpNed, &llaRef_);
        if (isfinite(lla.latitude) && isfinite(lla.longitude) && isfinite(vertex.x()) && isfinite(vertex.y()) && isfinite(vertex.z())) {
            out << lla.latitude << "," << lla.longitude << "," << vertex.z() << "," << vertex.x() << "," << vertex.y() << "," << vertex.z() << "\n";
        }
    }

    file.close();
}

void Surface::setData(const QVector<QVector3D>& data, int primitiveType)
{
    SceneObject::setData(data, primitiveType);

    updateGrid();
    updateContour();

    Q_EMIT changed();
}

void Surface::clearData()
{
    SceneObject::clearData();

    m_grid->clearData();
    m_contour->clearData();

    Q_EMIT changed();
}

void Surface::updateGrid()
{
    m_grid->clearData();

    switch(RENDER_IMPL(Surface)->primitiveType()){
    case GL_TRIANGLES:
        makeTriangleGrid();
        break;
    case GL_QUADS:{
        makeQuadGrid();

#if defined (Q_OS_ANDROID)
        auto impl = RENDER_IMPL(Surface);
        impl->quadSurfaceVertices_.clear();
        impl->quadSurfaceVertices_.reserve(impl->data().size() * 2);
        auto data = impl->data();
        for (int i = 0; i < data.size(); i += 4) {
            QVector3D A = data[i];
            QVector3D B = data[i + 1];
            QVector3D C = data[i + 2];
            QVector3D D = data[i + 3];
            impl->quadSurfaceVertices_.append({ A, B, C,
                                                A, C, D });
        }
#endif

        break;
    }
    default:
        break;
    }
}

void Surface::makeTriangleGrid()
{
    auto impl = RENDER_IMPL(Surface);

    if (impl->cdata().size() < 3)
        return;

    QVector <QVector3D> grid;

    for (int i = 0; i < impl->cdata().size()-3; i+=3){
        QVector3D A = impl->cdata()[i];
        QVector3D B = impl->cdata()[i+1];
        QVector3D C = impl->cdata()[i+2];

        A.setZ(A.z() + 0.03);
        B.setZ(B.z() + 0.03);
        C.setZ(C.z() + 0.03);

        grid.append({A, B,
                     B, C,
                     A, C});
    }

    m_grid->setData(grid, GL_LINES);
    impl->m_gridRenderImpl = *m_grid->m_renderImpl;
}

void Surface::makeQuadGrid()
{
    m_grid->clearData();

    auto impl = RENDER_IMPL(Surface);

    if (impl->cdata().size() < 4)
        return;

    QVector<QVector3D> grid;

    for (int i = 0; i < impl->cdata().size()-4; i+=4){
        QVector3D A = impl->cdata()[i];
        QVector3D B = impl->cdata()[i+1];
        QVector3D C = impl->cdata()[i+2];
        QVector3D D = impl->cdata()[i+3];

        A.setZ(A.z() + 0.03);
        B.setZ(B.z() + 0.03);
        C.setZ(C.z() + 0.03);
        D.setZ(D.z() + 0.03);

        grid.append({A, B,
                     B, C,
                     C, D,
                     A, D});
    }

    m_grid->setData(grid, GL_LINES);
    impl->m_gridRenderImpl = *m_grid->m_renderImpl;
}

void Surface::makeContourFromTriangles()
{
    auto impl = RENDER_IMPL(Surface);
    auto surfaceData = impl->cdata();

    BoundaryDetector <float> boundaryDetector;

    std::vector <::Triangle <float>> temp;
    for (int i = 0; i < surfaceData.size() - 3; i += 3) {
        temp.push_back({ Point3D<float>(surfaceData[i].x(),   surfaceData[i].y(),   surfaceData[i].z()),
                         Point3D<float>(surfaceData[i+1].x(), surfaceData[i+1].y(), surfaceData[i+1].z()),
                         Point3D<float>(surfaceData[i+2].x(), surfaceData[i+2].y(), surfaceData[i+2].z()) });
    }

    auto boundary = boundaryDetector.detect(temp);

    QVector<QVector3D> contour;

    for(const auto& segment : boundary){
        contour.append({segment.p1().toQVector3D(),
                          segment.p2().toQVector3D()
                    });
    };

    m_contour->setData(contour, GL_LINES);
    impl->m_contourRenderImpl = *m_contour->m_renderImpl;
}

void Surface::makeContourFromQuads()
{
    auto impl = RENDER_IMPL(Surface);
    auto surfaceData = impl->cdata();

    BoundaryDetector <float> boundaryDetector;

    std::vector <::Quad <float>> temp;

    for (int i = 0; i < surfaceData.size() - 4; i += 4) {
        temp.push_back(::Quad <float>(
                            Point3D <float>(surfaceData[i].x(),   surfaceData[i].y(),   surfaceData[i].z()),
                            Point3D <float>(surfaceData[i+1].x(), surfaceData[i+1].y(), surfaceData[i+1].z()),
                            Point3D <float>(surfaceData[i+2].x(), surfaceData[i+2].y(), surfaceData[i+2].z()),
                            Point3D <float>(surfaceData[i+3].x(), surfaceData[i+3].y(), surfaceData[i+3].z())
                        ));
    }

    auto boundary = boundaryDetector.detect(temp);

    QVector<QVector3D> contour;

    for(const auto& segment : boundary){
        contour.append({segment.p1().toQVector3D(),
                          segment.p2().toQVector3D()
                         });
    };

    m_contour->setData(contour, GL_LINES);
    impl->m_contourRenderImpl = *m_contour->m_renderImpl;
}

void Surface::updateContour()
{
    m_contour->clearData();

    switch(RENDER_IMPL(Surface)->primitiveType()){
    case GL_TRIANGLES:
        makeContourFromTriangles();
        break;
    case GL_QUADS:
        makeContourFromQuads();
        break;
    default:
        break;
    }
}

void Surface::SurfaceRenderImplementation::render(QOpenGLFunctions *ctx, const QMatrix4x4 &mvp, const QMap<QString, std::shared_ptr<QOpenGLShaderProgram>> &shaderProgramMap) const
{
    if(!m_isVisible)
        return;

    if(!shaderProgramMap.contains("height"))
        return;

    auto shaderProgram = shaderProgramMap["height"];

    if (!shaderProgram->bind()){
        qCritical() << "Error binding shader program.";
        return;
    }

    int posLoc    = shaderProgram->attributeLocation("position");
    int maxZLoc   = shaderProgram->uniformLocation("max_z");
    int minZLoc   = shaderProgram->uniformLocation("min_z");
    int matrixLoc = shaderProgram->uniformLocation("matrix");

    shaderProgram->setUniformValue(maxZLoc, m_bounds.maximumZ());
    shaderProgram->setUniformValue(minZLoc, m_bounds.minimumZ());
    shaderProgram->setUniformValue(matrixLoc, mvp);
    shaderProgram->enableAttributeArray(posLoc);

#if defined (Q_OS_ANDROID)
    if (primitiveType() == GL_TRIANGLES) {
        shaderProgram->setAttributeArray(posLoc, m_data.constData());
        ctx->glDrawArrays(m_primitiveType, 0, m_data.size());
    }
    else if (primitiveType() == GL_QUADS) {
        shaderProgram->setAttributeArray(posLoc, quadSurfaceVertices_.constData());
        ctx->glDrawArrays(GL_TRIANGLES, 0, quadSurfaceVertices_.size());
    }
#else
    shaderProgram->setAttributeArray(posLoc, m_data.constData());
    ctx->glDrawArrays(m_primitiveType, 0, m_data.size());
#endif

    shaderProgram->disableAttributeArray(posLoc);
    shaderProgram->release();

    m_gridRenderImpl.render(ctx, mvp, shaderProgramMap);
    m_contourRenderImpl.render(ctx, mvp, shaderProgramMap);
}
