#include "Q3DSceneModel.h"

Q3DSceneModel::Q3DSceneModel(std::shared_ptr <BottomTrackProvider> bottomTrackProvider,
                             QObject *parent)
:QObject(parent),
 mpBottomTrackProvider(bottomTrackProvider)
{
    mIsProcessingAvailable.store(true);
}

void Q3DSceneModel::setBottomTrack(const QVector <QVector3D>& bottomTrack)
{
    QMutexLocker locker(&mBottomTrackMutex);

    if (mpBottomTrackFilter){
        QVector <QVector3D> filtered;
        mpBottomTrackFilter->apply(bottomTrack, filtered);
        mBottomTrackDisplayedObject.setData(filtered);
    }else{
        mBottomTrackDisplayedObject.setData(bottomTrack);
    }

    Q_EMIT bottomTrackDataChanged();
}

void Q3DSceneModel::setBottomTrackFilter(std::shared_ptr<AbstractBottomTrackFilter> filter)
{
    mpBottomTrackFilter = filter;

    auto track = mpBottomTrackProvider->getBottomTrack();

    setBottomTrack(track);
}

void Q3DSceneModel::changeSceneVisibility(const bool visible)
{
    mSceneVisible = visible;

    Q_EMIT stateChanged();
}

void Q3DSceneModel::changeBottomTrackVisibility(const bool visible)
{
    mBottomTrackDisplayedObject.setVisible(visible);

    Q_EMIT bottomTrackPropertiesChanged();
}

void Q3DSceneModel::changeSurfaceVisibility(const bool visible)
{
    mSurfaceDisplayedObject.setVisible(visible);

    Q_EMIT surfacePropertiesChanged();
}

void Q3DSceneModel::changeSurfaceGridVisibility(const bool visible)
{
    mSurfaceDisplayedObject.setGridVisible(visible);

    Q_EMIT surfacePropertiesChanged();
}

void Q3DSceneModel::changeContourVisibility(const bool visible)
{
    mContourDisplayedObject.setVisible(visible);

    Q_EMIT contourPropertiesChanged();
}

void Q3DSceneModel::changeContourColor(QColor color)
{
    mContourDisplayedObject.setColor(color);

    Q_EMIT contourPropertiesChanged();
}

void Q3DSceneModel::changeContourLineWidth(float width)
{
    mContourDisplayedObject.setLineWidth(width);

    Q_EMIT contourPropertiesChanged();
}

void Q3DSceneModel::changeContourKeyPointsVisibility(bool visible)
{
    mContourDisplayedObject.setKeyPointsVisible(visible);

    Q_EMIT contourPropertiesChanged();
}

void Q3DSceneModel::changeContourKeyPointsColor(QColor color)
{
    mContourDisplayedObject.setKeyPointsColor(color);

    Q_EMIT contourPropertiesChanged();
}

void Q3DSceneModel::changeCalculationMethod(const QString& method)
{
    if (mParams.basicProcessingMethod() == method)
        return;

    mParams.setBasicProcessingMethod(method);
    mParams.setSmoothingMethod(SMOOTHING_METHOD_NONE);
}

void Q3DSceneModel::changeSmoothingMethod(const QString& method)
{
    if (mParams.smoothingMethod() == method)
        return;

    mParams.setSmoothingMethod(method);
}

void Q3DSceneModel::setTriangulationEdgeLengthLimit(double length)
{
    mParams.setTriangulationEdgeLengthLimit(length);
}

void Q3DSceneModel::changeGridType(QString type)
{
    mParams.setGridType(type);
}

void Q3DSceneModel::changeGridCellSize(double size)
{
    mParams.setGridCellSideSize(size);
}

void Q3DSceneModel::changeMarkupGridCellSize(float size)
{
    QMutexLocker surfaceLocker(&mSurfaceMutex);

    auto bounds = mSurfaceDisplayedObject.bounds();

    QMutexLocker markupGridLocker(&mMarkupGridMutex);

    mMarkupGrid.setSize({
                            bounds.minimumX(),
                            bounds.minimumY(),
                            bounds.minimumZ()
                        },
                        bounds.width(),
                        bounds.length(),
                        size);

    Q_EMIT markupGridDataChanged();
}

void Q3DSceneModel::updateSurface()
{
    mIsProcessingAvailable.store(false);

    QMutexLocker locker(&mBottomTrackMutex);

    if (mBottomTrackDisplayedObject.cdata().isEmpty()){
        mIsProcessingAvailable.store(true);
        return;
    }

    auto track = mBottomTrackDisplayedObject.cdata();

    locker.unlock();

    Q_EMIT stateChanged();

    ProcessingCaseFactory factory;

    auto pCase = factory.createCase(mParams);

    if (!pCase) return;

    auto vobject = pCase->process(track, mParams);

    {
        QMutexLocker surfaceLocker(&mSurfaceMutex);
        mSurfaceDisplayedObject.setVertexObject(vobject);

        QMutexLocker contourLocker(&mContourMutex);
        mContourDisplayedObject.setVertexObject(pCase->contourVertexObject());

        QMutexLocker boundsLocker(&mBoundsMutex);
        mBounds = mSurfaceDisplayedObject.bounds();

        QMutexLocker markupGridLocker(&mMarkupGridMutex);
        mMarkupGrid.setSize({
                                mBounds.minimumX(),
                                mBounds.minimumY(),
                                mBounds.minimumZ()
                            },
                            mBounds.width(),
                            mBounds.length(),
                            2.0f);
    }

    mIsProcessingAvailable.store(true);

    Q_EMIT stateChanged();
    Q_EMIT surfaceDataChanged();
    Q_EMIT contourDataChanged();
    Q_EMIT markupGridDataChanged();
}

void Q3DSceneModel::clear()
{
    {
        QMutexLocker surfaceLocker(&mSurfaceMutex);
        mSurfaceDisplayedObject.clear();

        QMutexLocker bottomTrackLocker(&mBottomTrackMutex);
        mBottomTrackDisplayedObject.clear();

        QMutexLocker contourLocker(&mContourMutex);
        mContourDisplayedObject.clear();

        QMutexLocker markupGridLocker(&mMarkupGridMutex);
        mMarkupGrid.clear();

        QMutexLocker boundsLocker(&mBoundsMutex);
        mBounds = Cube(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    }

    Q_EMIT surfaceDataChanged();
    Q_EMIT contourDataChanged();
    Q_EMIT bottomTrackDataChanged();
    Q_EMIT markupGridDataChanged();
}

bool Q3DSceneModel::sceneVisibility()
{
    return mSceneVisible;
}

bool Q3DSceneModel::bottomTrackVisible()
{
    return mBottomTrackDisplayedObject.isVisible();
}

bool Q3DSceneModel::surfaceVisible()
{
    QMutexLocker surfaceLocker(&mSurfaceMutex);

    return mSurfaceDisplayedObject.isVisible();
}

bool Q3DSceneModel::surfaceGridVisible()
{
    QMutexLocker surfaceLocker(&mSurfaceMutex);

    return mSurfaceDisplayedObject.isGridVisible();
}

bool Q3DSceneModel::triangulationAvailable()
{
    return mIsProcessingAvailable.load();
}

QString Q3DSceneModel::calculationMethod() const
{
    return mParams.basicProcessingMethod();
}

QString Q3DSceneModel::smoothingMethod() const
{
    return mParams.smoothingMethod();
}

BottomTrack Q3DSceneModel::bottomTrackDisplayedObject()
{
    QMutexLocker locker(&mBottomTrackMutex);

    return mBottomTrackDisplayedObject;
}

Surface Q3DSceneModel::surfaceDisplayedObject()
{
    QMutexLocker locker(&mSurfaceMutex);

    return mSurfaceDisplayedObject;
}

Contour Q3DSceneModel::contourDisplayedObject()
{
    QMutexLocker locker(&mContourMutex);

    return mContourDisplayedObject;
}

MarkupGrid Q3DSceneModel::markupGridDisplayedObject()
{
    QMutexLocker locker(&mMarkupGridMutex);

    return mMarkupGrid;
}

Cube Q3DSceneModel::bounds()
{
    QMutexLocker locker(&mBoundsMutex);

    return mBounds;
}

QColor Q3DSceneModel::contourColor() const
{
    return mContourDisplayedObject.rgbColor();
}

float Q3DSceneModel::contourLineWidth() const
{
    return mContourDisplayedObject.lineWidth();
}

bool Q3DSceneModel::contourVisibility() const
{
    return mContourDisplayedObject.isVisible();
}

bool Q3DSceneModel::contourKeyPointsVisibility() const
{
    return mContourDisplayedObject.keyPointsVisible();
}

QColor Q3DSceneModel::contourKeyPointsColor() const
{
    return mContourDisplayedObject.keyPointsRgbColor();
}

int orientation(const QVector3D& p, const QVector3D& q, const QVector3D& r)
{
    double val = (static_cast <double> (q.y()) - static_cast <double> (p.y())) * (static_cast <double> (r.x()) - static_cast <double> (q.x())) -
              (static_cast <double> (q.x()) - static_cast <double> (p.x())) * (static_cast <double> (r.y()) - static_cast <double> (q.y()));

    if (val == 0.0f) return 0;
    return (val > 0.0f)? 1:2;
}

double distance(const QVector3D& p1, const QVector3D& p2)
{
    return (static_cast <double> (p1.x()) - static_cast <double> (p2.x()))*(static_cast <double> (p1.x()) - static_cast <double> (p2.x())) +
           (static_cast <double> (p1.y()) - static_cast <double> (p2.y()))*(static_cast <double> (p1.y()) - static_cast <double> (p2.y()));
}

void Q3DSceneModel::createContour()
{
    auto points = mSurfaceDisplayedObject.data();

    if (points.size() < 3)
        return;

    // Ищем крайнюю левую нижнюю точку
    float ymin = points.first().y(), min = 0;

    for (int i = 1; i < points.size(); i++)
    {
      int y = points[i].y();

      if ((y < ymin) || (ymin == y &&
          points[i].x() < points[min].x())){
         ymin = points[i].y(), min = i;
      }
    }

#if defined QT_DEBUG
    qDebug() << "Bottom left point -> " << points[min];
#endif

    // Перемещаем найденную точку в начало контейнера
    std::swap(points[0], points[min]);

    // Сортируем точки по наименьшему векторному произведению
    auto comparator = [&points](const QVector3D& p1, const QVector3D& p2){
        int o = orientation(points[0],p1,p2);
        if (o == 0)
            return (distance(points[0], p2) >= distance(points[0], p1)) ? false : true;
        return (o == 2) ? false : true;
    };

    std::sort(points.begin()+1, points.end(), comparator);

#if defined QT_DEBUG
    qDebug() << "Points count after sort -> " << points.size();
#endif

    int m = 1;
    for (int i=1; i<points.size(); i++)
    {
        while (i < points.size()-1 && orientation(points[0], points[i],
                                     points[i+1]) == 0)
           i++;

        points[m] = points[i];
        m++;
    }

#if defined QT_DEBUG
    qDebug() << "Points count after decimation -> " << m;
#endif

    if (m < 3) return;

    // Формируем выпуклую оболочку
    QVector <QVector3D> ch;

    ch.append({points[0], points[1],points[2]});

    for(int i = 2; i < m; i++){

        while ((ch.size() >= 2) && (orientation(ch[ch.size()-2], points[i], ch[ch.size()-1])!= 2)) {
            ch.pop_back();
        }
        ch.append(points[i]);
    }

#if defined QT_DEBUG
    qDebug() << "Convex hull size -> " << ch.size();
#endif

    mContourDisplayedObject.setPrimitiveType(GL_LINE_LOOP);
    mContourDisplayedObject.setData(ch);

    Q_EMIT contourDataChanged();
}
