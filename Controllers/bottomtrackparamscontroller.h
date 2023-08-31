#ifndef BOTTOMTRACKPARAMSCONTROLLER_H
#define BOTTOMTRACKPARAMSCONTROLLER_H

#include <QObject>

#include <bottomtrackprovider.h>
#include <activeobjectprovider.h>

class BottomTrackParamsController : public QObject
{
    Q_OBJECT

public:
    BottomTrackParamsController() = delete;

    virtual ~BottomTrackParamsController();

    BottomTrackParamsController(
            std::shared_ptr <ActiveObjectProvider> selectedObjectModel,
            std::shared_ptr <BottomTrackProvider> bottomTrackProvider,
            QObject *parent = nullptr
        );

    Q_INVOKABLE void changeBottomTrackVisibility(bool visible);

    Q_INVOKABLE void changeBottomTrackFilter(int type);

    Q_INVOKABLE void changeBottomTrackFiltrationMethod(QString method);

    Q_INVOKABLE void changeNearestPointFiltrationRange(float range);

    Q_INVOKABLE void changeMaxPointsFiltrationCount(int count);

private:

    std::shared_ptr <ActiveObjectProvider> mpActiveObjectProvider;
    std::shared_ptr <BottomTrackProvider> mpBottomTrackProvider;
};

#endif // BOTTOMTRACKPARAMSCONTROLLER_H
