#ifndef SCENEOBJECTSMENUCONTROLLER_H
#define SCENEOBJECTSMENUCONTROLLER_H

#include <QObject>
#include <QStandardItemModel>

#include <qmlcomponentcontroller.h>
#include <graphicsscene3dview.h>

class SceneObjectsMenuController : public QmlComponentController
{
    Q_OBJECT
    Q_PROPERTY(QStandardItemModel* objectListModel READ objectListModel CONSTANT)

public:
    SceneObjectsMenuController(GraphicsScene3dView* sceneView = nullptr,
                               QObject *parent = nullptr);

    /**
     * @brief "object list current index changed" event handler
     * @param[in] row - current row index
     */
    Q_INVOKABLE void onObjectListIndexChanged(int row);

    /**
     * @brief "object list item name changed" event handler
     * @param[in] row - current row index
     * @param[in] name - current item name
     */
    Q_INVOKABLE void onObjectListItemNameChanged(int row, const QString& name);

    /**
     * @brief "object list item remove button clicked" event handler
     * @param[in] row -
     */
    Q_INVOKABLE void onObjectListItemRemoveButtonClicked(int row);

    /**
     * @brief onAddObjectButtonClicked
     * @param type
     */
    Q_INVOKABLE void onAddObjectButtonClicked(SceneObject::SceneObjectType type);

    /**
     * @brief onLoaderComponentLoaded
     * @param url
     */
    Q_INVOKABLE void onLoaderComponentLoaded(QObject* component, const QUrl& url);

public Q_SLOTS:
    void setSceneView(GraphicsScene3dView* sceneView);

protected:
    virtual void findComponent() override;

private:
    QStandardItemModel* objectListModel() const;

private:
    GraphicsScene3dView* m_sceneView = nullptr;
    QStandardItemModel* m_objectListModel = nullptr;
};

#endif // SCENEOBJECTSMENUCONTROLLER_H
