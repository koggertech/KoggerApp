#ifndef NPDFILTERPARAMSCONTROLLER_H
#define NPDFILTERPARAMSCONTROLLER_H

#include <QObject>

#include <activeobjectprovider.h>
#include <bottomtrackprovider.h>

class NPDFilterParamsController : public QObject
{
    Q_OBJECT
public:
    explicit NPDFilterParamsController(std::shared_ptr <ActiveObjectProvider> activeObjectProvider,
                                       std::shared_ptr <BottomTrackProvider> bottomTrackProvider,
                                       QObject *parent = nullptr);

    Q_INVOKABLE void setDistance(float distance);

private:

    std::shared_ptr <ActiveObjectProvider> mActiveObjectProvider;
    std::shared_ptr <BottomTrackProvider> mBottomTrackProvider;
};

#endif // NPDFILTERPARAMSCONTROLLER_H
