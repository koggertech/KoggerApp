#pragma once

#include "qmlcomponentcontroller.h"

class ImageView;
class GraphicsScene3dView;
class ImageViewControlMenuController : public QmlComponentController
{
    Q_OBJECT
    Q_PROPERTY(ImageView* imageView READ getImageViewPtr CONSTANT)

public:
    explicit ImageViewControlMenuController(QObject *parent = nullptr);
    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

    Q_INVOKABLE void onVisibilityChanged(bool state);
    Q_INVOKABLE void onUseFilterChanged(bool state);
    Q_INVOKABLE void onUpdateClicked(const QString& imagePath, int x1, int y1, int x2, int y2, int z);

Q_SIGNALS:

protected:
    virtual void findComponent() override;

private:
    ImageView* getImageViewPtr() const;
    GraphicsScene3dView* m_graphicsSceneView;
};
