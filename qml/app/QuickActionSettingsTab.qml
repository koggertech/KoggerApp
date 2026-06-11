import QtQuick 2.15
import kqml_types 1.0

Column {
    id: page

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceMd

    KSwitch {
        width: parent.width; text: qsTr("Show connected devices")
        checked: page.store ? page.store.quickActionConnectionStatusEnabled : true
        onToggled: {
            if (!page.store) return
            page.store.quickActionConnectionStatusEnabled = checked
            if (!deviceManagerWrapper || !deviceManagerWrapper.devs)
                return
            for (var i = 0; i < deviceManagerWrapper.devs.length; ++i) {
                var d = deviceManagerWrapper.devs[i]
                if (d && d.devType !== 0) {
                    page.store.requestHotkeysReveal("connections")
                    break
                }
            }
        }
    }

    KSwitch {
        width: parent.width; text: qsTr("Show favorite layouts")
        checked: page.store ? page.store.quickActionFavoritesEnabled : true
        onToggled: {
            if (!page.store) return
            page.store.quickActionFavoritesEnabled = checked
            if (page.store.favoriteLayouts && page.store.favoriteLayouts.length > 0)
                page.store.requestHotkeysReveal("layouts")
        }
    }

    KSwitch {
        width: parent.width; text: qsTr("Show bottom track editing")
        checked: page.store ? page.store.quickActionBottomTrackEnabled : true
        onToggled: {
            if (!page.store) return
            page.store.quickActionBottomTrackEnabled = checked
            page.store.requestHotkeysReveal("bottomTrack")
        }
    }

    KSwitch {
        width: parent.width; text: qsTr("Show extra info button")
        checked: page.store ? page.store.quickActionExtraInfoEnabled : true
        onToggled: {
            if (!page.store) return
            page.store.quickActionExtraInfoEnabled = checked
            page.store.requestHotkeysReveal("extraInfo")
        }
    }

    KSwitch {
        width: parent.width; text: qsTr("Show profiles button")
        checked: page.store ? page.store.quickActionProfilesEnabled : true
        onToggled: {
            if (!page.store) return
            page.store.quickActionProfilesEnabled = checked
            page.store.requestHotkeysReveal("profiles")
        }
    }
}
