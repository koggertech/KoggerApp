#pragma once

#include <QObject>


namespace map {


class TileManager : public QObject
{
    Q_OBJECT
public:
    explicit TileManager(QObject *parent = nullptr);
    virtual ~TileManager();
private:
};

} // namespace map

