// -----------------------------------------------------------------------
// File: LayoutPicker.qml
// Description: Layout chooser with a wireframe preview of each arrangement
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import "."
import QtQuick.Layouts
import "../../Controls"
import "../../Theme"

Item {
    id: picker

    property int currentId: HomeLayouts.defaultId

    signal layoutSelected(int layoutId)

    // The grid grows a row for every three arrangements. Without this the
    // item keeps whatever height it was given and the rows beyond it draw
    // over whatever comes next.
    implicitHeight: grid.implicitHeight

    GridLayout {
        id: grid
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        columns: 3
        rowSpacing: Spacing.xl
        columnSpacing: Spacing.xl

        Repeater {
            model: HomeLayouts.entries

            delegate: ColumnLayout {
                id: entry
                required property var modelData

                readonly property bool current: picker.currentId === entry.modelData.id

                Layout.fillWidth: true
                spacing: Spacing.sm

                AppButton {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Metrics.px(28)
                    labelPadding: Spacing.sm
                    text: entry.modelData.label
                    buttonVariant: AppButton.Neutral
                    checkable: true
                    checked: entry.current
                    onClicked: picker.layoutSelected(entry.modelData.id)
                }

                LayoutPreview {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Metrics.px(74)
                    cells: entry.modelData.cells
                    highlighted: entry.current
                    onClicked: picker.layoutSelected(entry.modelData.id)
                }
            }
        }
    }

    component LayoutPreview: Rectangle {
        id: preview

        property var cells: []
        property bool highlighted: false

        signal clicked()

        readonly property int columnCount: preview.extent("x", "w")
        readonly property int rowCount: preview.extent("y", "h")

        function extent(originKey, sizeKey) {
            var maximum = 1
            for (var i = 0; i < preview.cells.length; ++i) {
                var cell = preview.cells[i]
                maximum = Math.max(maximum, cell[originKey] + cell[sizeKey])
            }
            return maximum
        }

        color: Colors.transparent
        border.width: Metrics.borderWidth
        border.color: preview.highlighted ? Colors.accent : Colors.neutral

        Repeater {
            model: preview.cells

            delegate: Rectangle {
                required property var modelData

                readonly property real cellWidth: preview.width / preview.columnCount
                readonly property real cellHeight: preview.height / preview.rowCount

                x: modelData.x * cellWidth
                y: modelData.y * cellHeight
                width: modelData.w * cellWidth
                height: modelData.h * cellHeight
                color: Colors.transparent
                border.width: Metrics.borderWidth
                border.color: Colors.neutral
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: preview.clicked()
        }
    }
}
