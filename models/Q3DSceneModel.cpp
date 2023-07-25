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

void Q3DSceneModel::setObjectsPicker(std::shared_ptr<AbstractPicker> picker)
{
    mpObjectsPicker = picker;

    mPickedObject.clearData();

    bool needPicking =   mpObjectsPicker != nullptr
                     && (mSurfaceDisplayedObject.isVisible()
                     ||  mSurfaceDisplayedObject.isGridVisible());

    if (needPicking){
        auto surface = dynamic_cast <VertexObject*>(&mBottomTrackDisplayedObject);

        mPickedObject = mpObjectsPicker->pick(*surface);
    }

    Q_EMIT pickedObjectsDataChanged();
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

VertexObject Q3DSceneModel::pickedObject()
{
    return mPickedObject;
}

QString Q3DSceneModel::pickingMethod() const
{
    if (!mpObjectsPicker)
        return PICKING_METHOD_NONE;

    return mpObjectsPicker->pickingMethod();
}

void Q3DSceneModel::createObject(QString type)
{
    // TODO! ObjectsFactory->create(type);

    //Q_EMIT dataChanged();
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

QStringList Q3DSceneModel::objectsList() const
{
    QStringList result;

    auto it = mObjectsList.begin();

    while(it != mObjectsList.end()){

        auto object = *it;

        result.append(object->id());

        it++;
    }

    return result;
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
