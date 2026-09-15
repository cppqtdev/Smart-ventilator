// -----------------------------------------------------------------------
// File: ContentAsvPair.qml
// Description: Home layout 7 - the adaptive target graph twice over
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// reference/layout-dual-graphics.png divides this area into four equal
// tiles, not a wide one over two. The target graph runs down the left, the
// settings it was read from sit top right and the lung below them, with an
// eight pixel gap between tiles rather than the sixteen used between panes.
//
import QtQuick
import QtQuick.Layouts
import "../../Theme"

GridLayout {
    id: content

    property var presenter
    property bool frozen: false

    signal patientClicked()

    columns: 2
    rowSpacing: Spacing.sm
    columnSpacing: Spacing.sm

    AsvTargetPanel {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.preferredWidth: 1
        Layout.preferredHeight: 1
        presenter: content.presenter
    }

    AsvNumericsPanel {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.preferredWidth: 1
        Layout.preferredHeight: 1
        presenter: content.presenter
        onPatientClicked: content.patientClicked()
    }

    AsvTargetPanel {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.preferredWidth: 1
        Layout.preferredHeight: 1
        presenter: content.presenter
    }

    LungPanel {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.preferredWidth: 1
        Layout.preferredHeight: 1
        presenter: content.presenter
        frozen: content.frozen
        onPatientClicked: content.patientClicked()
    }
}
