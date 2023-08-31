#ifndef GRAPHICSSCENEENTITY_H
#define GRAPHICSSCENEENTITY_H

#include <QObject>

class GraphicsSceneEntity : public QObject
{
    Q_OBJECT
public:
    explicit GraphicsSceneEntity(QObject *parent = nullptr);

signals:

};

#endif // GRAPHICSSCENEENTITY_H
