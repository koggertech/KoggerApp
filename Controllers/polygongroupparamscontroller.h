#ifndef POLYGONGROUPPARAMSCONTROLLER_H
#define POLYGONGROUPPARAMSCONTROLLER_H

#include <QObject>

#include <activeobjectprovider.h>
#include <sceneobjectslistmodel.h>
#include <polygonobject.h>
#include <QModelIndex>

class PolygonGroupParamsController : public QObject
{
    Q_OBJECT
public:
    explicit PolygonGroupParamsController(std::shared_ptr<ActiveObjectProvider> activeObjectProvider,
                                          QObject *parent = nullptr);

    virtual ~PolygonGroupParamsController();

    Q_INVOKABLE void addPolygon() const;

    Q_INVOKABLE void removePolygon(int index) const;

    Q_INVOKABLE void addPoint(int polygonIndex) const;

    Q_INVOKABLE void removePoint(int polygonIndex, int pointIndex) const;

    Q_INVOKABLE void updatePointCoord(int polygonIndex, int pointIndex, const QVector3D& point) const;

private:
    std::shared_ptr <ActiveObjectProvider> mActiveObjectProvider;

};

#endif // POLYGONGROUPPARAMSCONTROLLER_H
