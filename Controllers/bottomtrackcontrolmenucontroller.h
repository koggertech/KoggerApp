#ifndef BOTTOMTRACKCONTROLMENUCONTROLLER_H
#define BOTTOMTRACKCONTROLMENUCONTROLLER_H

#include <qstringlistmodel.h>

#include <memory>

#include <qmlcomponentcontroller.h>

class BottomTrack;
class GraphicsScene3dView;
class BottomTrackControlMenuController : public QmlComponentController
{
    Q_OBJECT
    Q_PROPERTY(BottomTrack* bottomTrack READ bottomTrack CONSTANT)
    Q_PROPERTY(QStringListModel* channelListModel READ channelListModel CONSTANT)


public:
    BottomTrackControlMenuController(QObject* parent = nullptr);

    virtual ~BottomTrackControlMenuController();

    Q_INVOKABLE void onVisibilityCheckBoxCheckedChanged(bool checked);
    Q_INVOKABLE void onVisibleChannelComboBoxIndexChanged(int index);
    Q_INVOKABLE void onRestoreBottomTrackButtonClicked();

    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

protected:
    virtual void findComponent() override;

private:
    BottomTrack* bottomTrack() const;
    QStringListModel* channelListModel() const;

private:
    GraphicsScene3dView* m_graphicsSceneView = nullptr;
};

#endif // BOTTOMTRACKCONTROLMENUCONTROLLER_H
