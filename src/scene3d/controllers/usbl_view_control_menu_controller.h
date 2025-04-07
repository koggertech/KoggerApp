#pragma once

#include <QColor>
#include <QThread>
#include "qml_component_controller.h"
//#include "usbl_view.h"


class UsblView;
class GraphicsScene3dView;
class UsblViewControlMenuController : public QmlComponentController
{
    Q_OBJECT
    Q_PROPERTY(UsblView* usblView READ getUsblViewPtr CONSTANT)

public:
    explicit UsblViewControlMenuController(QObject *parent = nullptr);
    void setGraphicsSceneView(GraphicsScene3dView* sceneView);
    Q_INVOKABLE void onUsblViewVisibilityCheckBoxCheckedChanged(bool checked);
    Q_INVOKABLE void onUpdateUsblViewButtonClicked();
    Q_INVOKABLE void onClearUsblViewButtonClicked();

Q_SIGNALS:

protected:
    virtual void findComponent() override;

private:
    UsblView* getUsblViewPtr() const;

private:
    GraphicsScene3dView* m_graphicsSceneView = nullptr;
};
