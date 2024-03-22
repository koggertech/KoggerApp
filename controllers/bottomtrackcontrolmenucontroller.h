#ifndef BOTTOMTRACKCONTROLMENUCONTROLLER_H
#define BOTTOMTRACKCONTROLMENUCONTROLLER_H

#include <qmlcomponentcontroller.h>

#include <memory>

class BottomTrack;
class GraphicsScene3dView;
class BottomTrackControlMenuController : public QmlComponentController
{
    Q_OBJECT
    Q_PROPERTY(BottomTrack* bottomTrack READ bottomTrack CONSTANT)
    Q_PROPERTY(QStringList channelList READ channelList CONSTANT)
    Q_PROPERTY(int visibleChannelIndex READ visibleChannelIndex CONSTANT)

public:
    BottomTrackControlMenuController(QObject* parent = nullptr);

    virtual ~BottomTrackControlMenuController();

    Q_INVOKABLE void onVisibilityCheckBoxCheckedChanged(bool checked);
    Q_INVOKABLE void onVisibleChannelComboBoxIndexChanged(int index);

    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

protected:
    virtual void findComponent() override;

private:
    BottomTrack* bottomTrack() const;
    QStringList channelList() const;
    int visibleChannelIndex() const;

private Q_SLOTS:
    void updateChannelList();

Q_SIGNALS:
    void channelListUpdated();

private:
    GraphicsScene3dView* m_graphicsSceneView = nullptr;
    QStringList m_channelList;

};

#endif // BOTTOMTRACKCONTROLMENUCONTROLLER_H
