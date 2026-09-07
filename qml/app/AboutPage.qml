import QtQuick 2.15
import kqml_types 1.0

Column {
    id: page

    required property var store

    width: parent ? parent.width : implicitWidth
    spacing: Tokens.spaceLg

    readonly property var components: [
        { name: "Qt",       version: "6.8.3",  license: "GNU LGPL v3",            file: "COPYING.LGPLv3" },
        { name: "FFmpeg",   version: "7.1",    license: "GNU LGPL v2.1 or later", file: "COPYING.LGPLv2.1" },
        { name: "FreeType", version: "2.13.2", license: qsTr("The FreeType License (FTL)"), file: "FTL.TXT" }
    ]

    AppIdentityCard {
        anchors.horizontalCenter: parent.horizontalCenter
        store: page.store
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        visible: appUtils.gitRevision.length > 0
        text: qsTr("Revision %1").arg(appUtils.gitRevision)
        color: AppPalette.textSecond
        font.pixelSize: Tokens.fontXs
    }

    KIsland {
        title: qsTr("License")
        footer: qsTr("KoggerApp is distributed under the GNU General Public License v3.")

        KIslandRow {
            label: qsTr("Application license")
            caption: "GNU GPL v3"
            labelColor: AppPalette.textStrong
            interactive: true
            chevron: true
            onClicked: if (page.store) page.store.openLicenseView(qsTr("Application license"), "COPYING.GPLv3")
        }
    }

    KIsland {
        title: qsTr("Third-party components")
        footer: qsTr("Full notices, component provenance and where to obtain the sources are in "
                   + "THIRD_PARTY_NOTICES.md in the source tree.")

        Repeater {
            model: page.components
            delegate: KIslandRow {
                required property var modelData

                label: modelData.name + " " + modelData.version
                caption: modelData.license
                labelColor: AppPalette.textStrong
                interactive: true
                chevron: true
                onClicked: if (page.store) page.store.openLicenseView(label, modelData.file)
            }
        }
    }
}
