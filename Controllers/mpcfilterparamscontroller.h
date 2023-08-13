#ifndef MPCFILTERPARAMSCONTROLLER_H
#define MPCFILTERPARAMSCONTROLLER_H

#include <QObject>

#include <activeobjectprovider.h>
#include <bottomtrackprovider.h>

class MPCFilterParamsController : public QObject
{
    Q_OBJECT
public:
    explicit MPCFilterParamsController(std::shared_ptr <ActiveObjectProvider> activeObjectProvider,
                                       std::shared_ptr <BottomTrackProvider> bottomTrackProvider,
                                       QObject *parent = nullptr);

    Q_INVOKABLE void setMaxPointsCount(int count);

private:

    std::shared_ptr <ActiveObjectProvider> mActiveObjectProvider;
    std::shared_ptr <BottomTrackProvider> mBottomTrackProvider;
};

#endif // MPCFILTERPARAMSCONTROLLER_H
