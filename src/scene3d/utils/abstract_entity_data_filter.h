#ifndef ABSTRACTENTITYDATAFILTER_H
#define ABSTRACTENTITYDATAFILTER_H

#include <cstdint>
#include <QObject>
#include <QVector>
#include <QVector3D>

class AbstractEntityDataFilter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(FilterType type READ type CONSTANT)

public:
    explicit AbstractEntityDataFilter(QObject* parent = nullptr) : QObject(parent) {};
    ~AbstractEntityDataFilter() override = default;

    enum FilterType : uint8_t {
        Unknown = 0,
        MaxPointsCount = 1,
        NearestPointDistance = 2,
    };

    Q_ENUM(FilterType)

    virtual FilterType type() const = 0;
    virtual void apply(const QVector <QVector3D>& origin, QVector <QVector3D>& filtered) = 0;
};

#endif // ABSTRACTENTITYDATAFILTER_H
