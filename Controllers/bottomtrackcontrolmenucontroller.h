#ifndef BOTTOMTRACKCONTROLMENUCONTROLLER_H
#define BOTTOMTRACKCONTROLMENUCONTROLLER_H

#include <memory>

#include <qmlcomponentcontroller.h>

class BottomTrack;
class GraphicsScene3dView;
class BottomTrackProvider;
class BottomTrackControlMenuController : public QmlComponentController
{
    Q_OBJECT
    Q_PROPERTY(BottomTrack* bottomTrack READ bottomTrack CONSTANT)

public:
    BottomTrackControlMenuController(QObject* parent = nullptr);

    virtual ~BottomTrackControlMenuController();

    Q_INVOKABLE void onVisibilityCheckBoxCheckedChanged(bool checked);

    Q_INVOKABLE void onFilterTypeComboBoxIndexChanged(int index);

    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

    void setBottomTrackProvider(std::shared_ptr <BottomTrackProvider> provider);

protected:
    virtual void findComponent() override;

private:
    BottomTrack* bottomTrack() const;

private:
    GraphicsScene3dView* m_graphicsSceneView = nullptr;
    std::shared_ptr <BottomTrackProvider> m_bottomTrackProvider;
};

#endif // BOTTOMTRACKCONTROLMENUCONTROLLER_H
