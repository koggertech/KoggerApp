#ifndef BOTTOMTRACK_H
#define BOTTOMTRACK_H

#include <memory>
#include <displayedobject.h>
#include <abstractbottomtrackfilter.h>

class BottomTrack : public DisplayedObject
{
    Q_OBJECT

    Q_PROPERTY(AbstractBottomTrackFilter* filter READ filter NOTIFY filterChanged)

public:

    explicit BottomTrack(QObject* parent = nullptr);

    virtual ~BottomTrack();

    void setFilter(std::shared_ptr <AbstractBottomTrackFilter> filter);

    /**
     * @brief Returns length of the bottom track
     * @return Length of the route
     */
    float routeLength() const;

    AbstractBottomTrackFilter* filter() const;

    //! @brief Устанавливает набор вершин объекта.
    //! @param[in] data - ссылка на набор вершин.
    virtual void setData(const QVector <QVector3D>& data) override;

    virtual SceneObjectType type() const override;

signals:

    void filterChanged(AbstractBottomTrackFilter* filter);

private:

    std::shared_ptr <AbstractBottomTrackFilter> mpFilter;
};

#endif // BOTTOMTRACK_H
