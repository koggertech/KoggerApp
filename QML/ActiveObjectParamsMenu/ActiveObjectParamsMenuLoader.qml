import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.1

import SceneObject 1.0

Item {
    id: root
    Layout.minimumWidth:  160
    Layout.minimumHeight: 240

    property var activeObject: null

    function setActiveObject(object){
        if(object.type === SceneObject.BottomTrack){
            loader.setSource("BottomTrackParamsMenu.qml")
        }else if(object.type === SceneObject.Surface){
            loader.setSource("SurfaceParamsMenu.qml")
        }else if(object.type === SceneObject.PointGroup){
            loader.setSource("PointGroupParamsMenu.qml")
        }else if(object.type === SceneObject.PolygonGroup){
            loader.setSource("PolygonGroupParamsMenu.qml")
        }else{
            loader.setSource("MenuPlaceholder.qml")
        }

        activeObject = object

        if(activeObject)
            loader.item.setActiveObject(activeObject)
    }

    function reset(){
        loader.setSource("MenuPlaceholder.qml")
    }

    Loader {
        id: loader
        anchors.fill: parent
        source: "MenuPlaceholder.qml"

        onLoaded: {
            //binder.target.setActiveObject(item)
            //binder.target = item
        }

    }

    Binding {
        id:       binder
        property: "activeObject"
        value:    activeObject
    }
}
