#ifndef SURFACE_H
#define SURFACE_H

#include <memory>

#include <scenegraphicsobject.h>
#include <contour.h>
#include <surfacegrid.h>
#include <constants.h>

class Surface : public SceneGraphicsObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT("Surface")
    Q_PROPERTY(Contour* contour  READ contour CONSTANT)
    Q_PROPERTY(SurfaceGrid* grid READ grid    CONSTANT)

public:

    explicit Surface(QObject* parent = nullptr);

    virtual ~Surface();

    Contour* contour() const;

    /**
     * @brief Returns pointer to surface grid object
     * @return Pointer to surface grid object
     */
    SurfaceGrid* grid() const;

    //! @brief Устанавливает набор вершин объекта.
    //! @param[in] data - ссылка на набор вершин.
    virtual void setData(const QVector <QVector3D>& data) override;

    virtual void clearData() override;

    virtual void draw(QOpenGLFunctions* ctx,
                      const QMatrix4x4& mvp,
                      const QMap <QString, std::shared_ptr <QOpenGLShaderProgram>>& shaderProgramMap) const override;

    virtual SceneObjectType type() const override;

    //! @brief Добавляет вершину в конец набора вершин.
    //! @param[in] vertex - ссылка на вершину
    virtual void append(const QVector3D& vertex) override;

    //! @brief Добавляет входящий набор вершин в конец набора вершин объекта
    //! @param[in] other - ссылка на набор вершин
    virtual void append(const QVector<QVector3D>& other) override;

private:

    void updateContour();

    void updateGrid();

    void makeTriangleGrid();

    void makeQuadGrid();

    void makeContourFromTriangles();

    void makeContourFromQuads();

    void drawSurface(QOpenGLFunctions* ctx, const QMatrix4x4& mvp, QOpenGLShaderProgram* shaderProgram) const;

    void drawContour(QOpenGLFunctions* ctx, const QMatrix4x4& mvp, QOpenGLShaderProgram* shaderProgram) const;

    void drawGrid(QOpenGLFunctions* ctx, const QMatrix4x4& mvp, QOpenGLShaderProgram* shaderProgram) const;

signals:

    void gridChanged();

    void contourChanged();

private:
    QString m_bottomTrackId = "";
    std::shared_ptr <Contour> m_contour;
    std::shared_ptr <SurfaceGrid> m_grid;
};

#endif // SURFACE_H
