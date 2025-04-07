#pragma once

#include "qml_component_controller.h"

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
    Q_INVOKABLE void onUpdateClicked(const QString& imagePath, double lat_lt, double lon_lt, double lat_rb, double lon_rb, float z);

Q_SIGNALS:

protected:
    virtual void findComponent() override;

private:
    ImageView* getImageViewPtr() const;
    GraphicsScene3dView* m_graphicsSceneView;
};
