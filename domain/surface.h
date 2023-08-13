#ifndef SURFACE_H
#define SURFACE_H

#include <displayedobject.h>
#include <bottomtrack.h>
#include <contour.h>
#include <surfacegrid.h>
#include <constants.h>
#include <Edge.h>

class Surface : public DisplayedObject
{
    Q_OBJECT

    Q_PROPERTY(QString bottomTrackId READ bottomTrackId WRITE setBottomTrackId NOTIFY bottomTrackIdChanged)
    Q_PROPERTY(Contour* contour      READ contour                              NOTIFY contourChanged)
    Q_PROPERTY(SurfaceGrid* grid     READ grid                                 NOTIFY gridChanged)

public:

    explicit Surface(QObject* parent = nullptr);

    virtual ~Surface();

    Q_INVOKABLE Contour* contour() const;

    /**
     * @brief Returns pointer to surface grid object
     * @return Pointer to surface grid object
     */
    Q_INVOKABLE SurfaceGrid* grid() const;

public:

    //! @brief Устанавливает набор вершин объекта.
    //! @param[in] data - ссылка на набор вершин.
    virtual void setData(const QVector <QVector3D>& data) override;

    //! @brief Добавляет вершину в конец набора вершин.
    //! @param[in] vertex - ссылка на вершину
    virtual void append(const QVector3D& vertex) override;

    //! @brief Добавляет входящий набор вершин в конец набора вершин объекта
    //! @param[in] other - ссылка на набор вершин
    virtual void append(const QVector<QVector3D>& other) override;

    /**
     * @brief  Returns id of source bottom track object
     * @return Id of source bottom track object
     */
    QString bottomTrackId() const;

    /**
     * @brief Sets id of source bottom track object
     * @param id Id of source bottom track object
     */
    void setBottomTrackId(QString id);

private:

    void updateContour();

    void updateGrid();

    void makeTriangleGrid();

    void makeQuadGrid();

    void makeContourFromTriangles();

    void makeContourFromQuads();

signals:

    void bottomTrackIdChanged(QString id);

    void gridChanged();

    void contourChanged();

private:

    QString mBasicCalculationMethod = "TIN";
    QString mSmoothingMethod = "None";
    QString mBottomTrackId = "";

    std::shared_ptr <Contour> mContour;

    std::shared_ptr <SurfaceGrid> mGrid;

};

#endif // SURFACE_H
