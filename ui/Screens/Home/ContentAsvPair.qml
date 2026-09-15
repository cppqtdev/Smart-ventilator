// -----------------------------------------------------------------------
// File: ContentAsvPair.qml
// Description: Home layout 7 - the adaptive target graph twice over
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// The reference stacks the target graph over itself, the upper one carrying
// the settings the target was derived from and the lower one the patient it
// was derived for.
//
import QtQuick
import QtQuick.Layouts
import "../../Components"
import "../../Theme"

ColumnLayout {
    id: content

    property var presenter
    property bool frozen: false

    signal patientClicked()

    spacing: Metrics.gutter

    AsvTargetPanel {
        Layout.fillWidth: true
        Layout.fillHeight: true
        presenter: content.presenter
        showSettings: true
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: Metrics.gutter

        AsvTargetPanel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            presenter: content.presenter
        }

        LungPanel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            presenter: content.presenter
            frozen: content.frozen
            onPatientClicked: content.patientClicked()
        }
    }
}
