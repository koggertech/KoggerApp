#ifndef BOTTOMTRACKCONTROLMENUCONTROLLER_H
#define BOTTOMTRACKCONTROLMENUCONTROLLER_H

#include <qmlcomponentcontroller.h>

#include <memory>

#include <QStandardItem>
#include <QStringListModel>

class BottomTrack;
class GraphicsScene3dView;
class BottomTrackControlMenuController : public QmlComponentController
{
    Q_OBJECT
    Q_PROPERTY(BottomTrack* bottomTrack READ bottomTrack CONSTANT)
    Q_PROPERTY(QStandardItemModel* channelListModel READ channelListModel CONSTANT)


public:
    BottomTrackControlMenuController(QObject* parent = nullptr);

    virtual ~BottomTrackControlMenuController();

    Q_INVOKABLE void onVisibilityCheckBoxCheckedChanged(bool checked);
    Q_INVOKABLE void onVisibleChannelComboBoxIndexChanged(int index);
    //Q_INVOKABLE void onRestoreBottomTrackButtonClicked();

    void setGraphicsSceneView(GraphicsScene3dView* sceneView);

protected:
    virtual void findComponent() override;

private:
    BottomTrack* bottomTrack() const;
    QStandardItemModel* channelListModel() const;

private Q_SLOTS:
    void updateChannelListModel();

private:
    GraphicsScene3dView* m_graphicsSceneView = nullptr;
    std::unique_ptr <QStandardItemModel> m_channelListModel;
};

#endif // BOTTOMTRACKCONTROLMENUCONTROLLER_H
