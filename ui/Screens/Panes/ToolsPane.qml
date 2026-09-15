// -----------------------------------------------------------------------
// File: ToolsPane.qml
// Description: Tools screen - P/V Tool, configuration, hold and utilities
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Layouts
import "../../Controls"
import "../../Theme"

Item {
    id: pane

    property var ventilatorData
    property var alarmData
    property int pageIndex: 0
    property string cursor: ""

    signal inspiratoryHoldRequested()
    signal expiratoryHoldRequested()
    signal settingsRequested()

    property bool showReference: false
    property real cursorPressure: 0

    readonly property bool pvRunning:
        pane.ventilatorData ? pane.ventilatorData.pvToolRunning : false

    readonly property var pvInflation:
        pane.ventilatorData ? pane.ventilatorData.pvInflationLimb : []

    readonly property var pvDeflation:
        pane.ventilatorData ? pane.ventilatorData.pvDeflationLimb : []

    readonly property var pvOutcome:
        pane.ventilatorData ? pane.ventilatorData.pvResult : ({})

    function togglePvTool() {
        if (!pane.ventilatorData)
            return
        if (pane.pvRunning)
            pane.ventilatorData.stopPvTool()
        else
            pane.ventilatorData.startPvTool()
    }

    // The two rows of the limb table read the manoeuvre at the cursor
    // pressure and at the top of the curve. Before it has run there is
    // nothing to read, which is the row of dashes the reference shows.
    function sampleAt(limb, pressure) {
        for (var i = 0; i < limb.length; ++i) {
            if (limb[i].paw >= pressure)
                return limb[i]
        }
        return limb.length > 0 ? limb[limb.length - 1] : null
    }

    function limb(which, row) {
        var pressure = row === 0 ? pane.cursorPressure : 40
        if (which === "paw")
            return pane.pvInflation.length > 0 ? pressure : undefined
        var source = which === "inflation" ? pane.pvInflation : pane.pvDeflation
        var point = pane.sampleAt(source, pressure)
        return point === null ? undefined : point.volume
    }

    function result(key) {
        var outcome = pane.pvOutcome
        if (!outcome || outcome[key] === undefined)
            return undefined
        return outcome[key]
    }

    function limbAxis(field) {
        var out = []
        var i
        for (i = 0; i < pane.pvInflation.length; ++i)
            out.push(pane.pvInflation[i][field])
        for (i = 0; i < pane.pvDeflation.length; ++i)
            out.push(pane.pvDeflation[i][field])
        return out
    }

    readonly property var pages: [
        { key: "pv",       label: qsTr("P/V Tools") },
        { key: "config",   label: qsTr("Configuration") },
        { key: "hold",     label: qsTr("Hold") },
        { key: "utility",  label: qsTr("Utilities") }
    ]

    // Readings arrive as doubles, so they are rounded here. Printing one
    // raw puts 15.200000000000001 on a clinical screen.
    function reading(key, decimals) {
        if (!pane.ventilatorData)
            return "---"
        var value = pane.ventilatorData[key]
        if (value === undefined || value === null)
            return "---"
        var places = decimals === undefined ? 1 : decimals
        return isNaN(value) ? String(value) : Number(value).toFixed(places)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Metrics.gutter

        SubTabStrip {
            Layout.fillWidth: true
            model: pane.pages
            currentIndex: pane.pageIndex
            onActivated: function (index) { pane.pageIndex = index }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Metrics.gutter
            visible: pane.pageIndex === 0

            ColumnLayout {
                id: toolColumn

                // Chips default to their label width as a minimum, which is
                // what lets a two chip row push past the column and draw
                // over the loop chart. Inside this column they are allowed
                // to shrink and elide instead.
                readonly property int chipMinimum: Metrics.px(72)

                Layout.fillWidth: false
                Layout.preferredWidth: Metrics.px(258)
                Layout.maximumWidth: Metrics.px(258)
                Layout.alignment: Qt.AlignTop
                spacing: Spacing.md

                ChipButton {
                    Layout.fillWidth: true
                    Layout.minimumWidth: toolColumn.chipMinimum
                    text: qsTr("Reference")
                    selected: pane.showReference
                    onClicked: pane.showReference = !pane.showReference
                }

                DialControl {
                    Layout.fillWidth: true
                    label: ""
                    unit: "cmH2O"
                    from: 0
                    to: 60
                    decimals: 0
                    value: pane.cursorPressure
                    onStepRequested: function (proposed) { pane.cursorPressure = proposed }
                }

                // Inflation limb, deflation limb and the pressure each pair
                // was taken at, which is the table the reference draws under
                // the cursor dial.
                GridLayout {
                    Layout.fillWidth: true
                    columns: 3
                    columnSpacing: Spacing.sm
                    rowSpacing: Spacing.xs

                    LimbHeading { text: qsTr("Inflation Limb"); tint: Colors.success }
                    LimbHeading { text: qsTr("Deflation Limb"); tint: Colors.accent }
                    LimbHeading { text: qsTr("Paw");            tint: Colors.warning }

                    LimbCell { value: pane.limb("inflation", 0); unit: "ml" }
                    LimbCell { value: pane.limb("deflation", 0); unit: "ml" }
                    LimbCell { value: pane.limb("paw", 0);       unit: "cmH2O" }

                    LimbCell { value: pane.limb("inflation", 1); unit: "ml" }
                    LimbCell { value: pane.limb("deflation", 1); unit: "ml" }
                    LimbCell { value: pane.limb("paw", 1);       unit: "cmH2O" }

                    Text {
                        Layout.columnSpan: 3
                        Layout.topMargin: Spacing.xs
                        text: qsTr("Compliance")
                        color: Colors.textPrimary
                        font.family: Typography.monoFamily
                        font.pixelSize: Typography.readoutLabel
                    }

                    LimbCell { value: pane.result("cInflation"); unit: "ml/cmH2O" }
                    LimbCell { value: pane.result("cDeflation"); unit: "ml/cmH2O" }
                    Item { Layout.fillWidth: true }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Spacing.sm

                    ChipButton {
                        Layout.fillWidth: true
                        Layout.minimumWidth: toolColumn.chipMinimum
                        labelPadding: Spacing.xs
                        text: qsTr("Cursor 1")
                        selected: pane.cursor === "1"
                        onClicked: pane.cursor = "1"
                    }

                    ChipButton {
                        Layout.fillWidth: true
                        Layout.minimumWidth: toolColumn.chipMinimum
                        labelPadding: Spacing.xs
                        text: qsTr("Cursor 2")
                        selected: pane.cursor === "2"
                        onClicked: pane.cursor = "2"
                    }
                }

                Text {
                    text: qsTr("Current Settings")
                    color: Colors.textPrimary
                    font.family: Typography.monoFamily
                    font.pixelSize: Typography.readoutLabel
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Spacing.lg

                    SettingReadout {
                        label: qsTr("Ptop")
                        value: pane.reading("alarmHighPressure", 0)
                        unit: "cmH2O"
                    }

                    SettingReadout {
                        label: qsTr("Pcuff")
                        value: pane.reading("totalPeep", 0)
                        unit: "cmH2O"
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Spacing.sm

                    ChipButton {
                        Layout.fillWidth: true
                        Layout.minimumWidth: toolColumn.chipMinimum
                        labelPadding: Spacing.xs
                        text: pane.pvRunning ? qsTr("Stop") : qsTr("Start/Stop")
                        selected: pane.pvRunning
                        onClicked: pane.togglePvTool()
                    }

                    ChipButton {
                        Layout.fillWidth: true
                        Layout.minimumWidth: toolColumn.chipMinimum
                        labelPadding: Spacing.xs
                        text: qsTr("Settings")
                        onClicked: pane.settingsRequested()
                    }
                }

                Item { Layout.fillHeight: true }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: Metrics.px(240)
                spacing: Spacing.sm

                LoopChart {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    title: qsTr("P-V Loop")
                    xLabel: "Paw cmH2O"
                    yLabel: "V ml"
                    xMinimum: 0
                    xMaximum: 40
                    yMinimum: 0
                    yMaximum: 1000
                    xSamples: pane.limbAxis("paw")
                    ySamples: pane.limbAxis("volume")
                }

                // The points the manoeuvre is run for: where the lung starts
                // opening, where it stops taking volume, and what the set
                // pressure is holding at the end of expiration.
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Spacing.md

                    PointReadout { label: qsTr("LIP"); glyph: "\u25BC"; value: pane.result("lip");   unit: "cmH2O" }
                    PointReadout { label: qsTr("PDR"); glyph: "\u25CF"; value: pane.result("pdr");   unit: "cmH2O" }
                    PointReadout { label: qsTr("UIP"); glyph: "\u25B2"; value: pane.result("uip");   unit: "cmH2O" }
                    PointReadout { label: qsTr("Vpeep"); glyph: "";      value: pane.result("vpeep"); unit: "ml" }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Spacing.lg
            visible: pane.pageIndex === 2

            AppButton {
                Layout.preferredWidth: Math.max(Metrics.px(170), implicitWidth)
                Layout.preferredHeight: Metrics.px(30)
                text: qsTr("Inspiratory hold")
                buttonVariant: AppButton.Primary
                enabled: pane.ventilatorData
                         && pane.ventilatorData.running
                         && !pane.ventilatorData.holdInProgress
                onClicked: pane.inspiratoryHoldRequested()
            }

            AppButton {
                Layout.preferredWidth: Math.max(Metrics.px(170), implicitWidth)
                Layout.preferredHeight: Metrics.px(30)
                text: qsTr("Expiratory hold")
                buttonVariant: AppButton.Primary
                enabled: pane.ventilatorData
                         && pane.ventilatorData.running
                         && !pane.ventilatorData.holdInProgress
                onClicked: pane.expiratoryHoldRequested()
            }

            Item { Layout.fillHeight: true }
        }

        UtilitiesPane {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: pane.pageIndex === 3
            ventilatorData: pane.ventilatorData
            alarmData: pane.alarmData
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: pane.pageIndex === 1

            Text {
                anchors.centerIn: parent
                text: qsTr("No entries.")
                color: Colors.textSecondary
                font.family: Typography.monoFamily
                font.pixelSize: Typography.readoutLabel
            }
        }
    }

    // The three coloured column headings over the limb table.
    component LimbHeading: Text {
        property color tint: Colors.textPrimary

        Layout.fillWidth: true
        color: tint
        font.family: Typography.monoFamily
        font.pixelSize: Typography.caption
        elide: Text.ElideRight
    }

    // One measured value with its unit under it. An absent value reads as
    // dashes, because the manoeuvre has not produced it yet.
    component LimbCell: ColumnLayout {
        id: cell

        property var value: undefined
        property string unit: ""
        property int decimals: 1

        Layout.fillWidth: true
        spacing: 0

        Text {
            text: cell.value === undefined || cell.value === null
                  ? "---" : Number(cell.value).toFixed(cell.decimals)
            color: Colors.textPrimary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.readoutLabel
            font.weight: Typography.bold
        }

        Text {
            text: cell.unit
            color: Colors.textSecondary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.micro
        }
    }

    // Label over a large value over its unit, as Current Settings is drawn.
    component SettingReadout: ColumnLayout {
        id: setting

        property string label: ""
        property string value: "---"
        property string unit: ""

        spacing: 0

        Text {
            text: setting.label
            color: Colors.textPrimary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.readoutLabel
        }

        Text {
            text: setting.value
            color: Colors.textPrimary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.readoutValue
            font.weight: Typography.bold
        }

        Text {
            text: setting.unit
            color: Colors.textPrimary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.readoutUnit
        }
    }

    // One derived point under the chart: a marker, its name, its value.
    component PointReadout: ColumnLayout {
        id: point

        property string label: ""
        property string glyph: ""
        property var value: undefined
        property string unit: ""

        Layout.fillWidth: true
        spacing: 0

        Text {
            text: point.glyph.length > 0
                  ? point.label + " " + point.glyph : point.label
            color: Colors.textSecondary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.caption
        }

        Text {
            text: point.value === undefined || point.value === null
                  ? "---" : Number(point.value).toFixed(0)
            color: Colors.textPrimary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.readoutLabel
            font.weight: Typography.bold
        }

        Text {
            text: point.unit
            color: Colors.textSecondary
            font.family: Typography.monoFamily
            font.pixelSize: Typography.micro
        }
    }
}
