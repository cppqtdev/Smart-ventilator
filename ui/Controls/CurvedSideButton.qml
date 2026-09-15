// -----------------------------------------------------------------------
// File: CurvedSideButton.qml
// Description: Shape-based curved button for gauge side controls
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes

import "../Theme"

Item {
    id: root

    enum Edge {
        Left,
        Right,
        Top,
        Bottom
    }

    property int edge: CurvedSideButton.Left

    property color backgroundColor: Colors.accentCyan
    property color borderColor: "transparent"
    property real borderWidth: 0

    property real cornerRadius: 12
    property real curveDepth: 50

    property string text: ""
    property color textColor: "white"
    property int textPixelSize: 24

    signal clicked()

    readonly property alias pressed: tap.pressed

    /*  With no pointer there is no hover to say what a shape will do, so the
        press is the only feedback a tap ever gets. The whole shape carries
        it, not just the label. */
    readonly property color activeFill: root.pressed ? Qt.darker(root.backgroundColor, 1.18)
                                                     : root.backgroundColor

    width: Math.max(160, Metrics.touchTarget)
    height: Math.max(220, Metrics.touchTarget)
    opacity: root.enabled ? 1.0 : 0.45

    TapHandler {
        id: tap

        onTapped: root.clicked()
    }

    Loader {
        anchors.fill: parent

        sourceComponent: {
            switch(root.edge) {
            case CurvedSideButton.Left:
                return leftShape

            case CurvedSideButton.Right:
                return rightShape

            case CurvedSideButton.Top:
                return topShape

            case CurvedSideButton.Bottom:
                return bottomShape
            }

            return leftShape
        }
    }


    Text {
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: root.edge === CurvedSideButton.Top
                                  || root.edge === CurvedSideButton.Bottom
                                  ? parent.horizontalCenter : undefined
        anchors.left: root.edge === CurvedSideButton.Left ? parent.left : undefined
        anchors.right: root.edge === CurvedSideButton.Right ? parent.right : undefined
        anchors.margins: Metrics.px(14)

        text: root.text
        color: root.textColor
        font.family: Typography.family
        font.pixelSize: root.textPixelSize
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    //
    // LEFT
    //

    Component {
        id: leftShape

        Shape {
            anchors.fill: parent
            antialiasing: true
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                fillColor: root.activeFill
                strokeColor: root.borderColor
                strokeWidth: root.borderWidth

                startX: root.cornerRadius
                startY: 0

                PathQuad {
                    x: 0
                    y: root.cornerRadius
                    controlX: 0
                    controlY: 0
                }

                PathLine {
                    x: 0
                    y: root.height - root.cornerRadius
                }

                PathQuad {
                    x: root.cornerRadius
                    y: root.height
                    controlX: 0
                    controlY: root.height
                }

                PathLine {
                    x: root.width
                    y: root.height
                }

                PathCubic {
                    x: root.width
                    y: 0

                    control1X: root.curveDepth
                    control1Y: root.height * 0.75

                    control2X: root.curveDepth
                    control2Y: root.height * 0.25
                }

                PathLine {
                    x: root.cornerRadius
                    y: 0
                }
            }
        }
    }

    //
    // RIGHT
    //

    Component {
        id: rightShape

        Shape {
            anchors.fill: parent
            antialiasing: true
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                fillColor: root.activeFill
                strokeColor: root.borderColor
                strokeWidth: root.borderWidth

                startX: root.width - root.cornerRadius
                startY: 0

                PathQuad {
                    x: root.width
                    y: root.cornerRadius
                    controlX: root.width
                    controlY: 0
                }

                PathLine {
                    x: root.width
                    y: root.height - root.cornerRadius
                }

                PathQuad {
                    x: root.width - root.cornerRadius
                    y: root.height
                    controlX: root.width
                    controlY: root.height
                }

                PathLine {
                    x: 0
                    y: root.height
                }

                PathCubic {
                    x: 0
                    y: 0

                    control1X: root.width - root.curveDepth
                    control1Y: root.height * 0.75

                    control2X: root.width - root.curveDepth
                    control2Y: root.height * 0.25
                }

                PathLine {
                    x: root.width - root.cornerRadius
                    y: 0
                }
            }
        }
    }

    //
    // TOP
    //

    Component {
        id: topShape

        Shape {
            anchors.fill: parent
            antialiasing: true
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                fillColor: root.activeFill
                strokeColor: root.borderColor
                strokeWidth: root.borderWidth

                startX: 0
                startY: root.cornerRadius

                PathQuad {
                    x: root.cornerRadius
                    y: 0
                    controlX: 0
                    controlY: 0
                }

                PathCubic {
                    x: root.width - root.cornerRadius
                    y: 0

                    control1X: root.width * 0.30
                    control1Y: root.curveDepth

                    control2X: root.width * 0.70
                    control2Y: root.curveDepth
                }

                PathQuad {
                    x: root.width
                    y: root.cornerRadius
                    controlX: root.width
                    controlY: 0
                }

                PathLine {
                    x: root.width
                    y: root.height
                }

                PathLine {
                    x: 0
                    y: root.height
                }

                PathLine {
                    x: 0
                    y: root.cornerRadius
                }
            }
        }
    }

    //
    // BOTTOM
    //

    Component {
        id: bottomShape

        Shape {
            anchors.fill: parent
            antialiasing: true
            preferredRendererType: Shape.CurveRenderer

            ShapePath {
                fillColor: root.activeFill
                strokeColor: root.borderColor
                strokeWidth: root.borderWidth

                startX: 0
                startY: 0

                PathLine {
                    x: root.width
                    y: 0
                }

                PathLine {
                    x: root.width
                    y: root.height - root.cornerRadius
                }

                PathQuad {
                    x: root.width - root.cornerRadius
                    y: root.height
                    controlX: root.width
                    controlY: root.height
                }

                PathCubic {
                    x: root.cornerRadius
                    y: root.height

                    control1X: root.width * 0.70
                    control1Y: root.height - root.curveDepth

                    control2X: root.width * 0.30
                    control2Y: root.height - root.curveDepth
                }

                PathQuad {
                    x: 0
                    y: root.height - root.cornerRadius
                    controlX: 0
                    controlY: root.height
                }

                PathLine {
                    x: 0
                    y: 0
                }
            }
        }
    }
}
