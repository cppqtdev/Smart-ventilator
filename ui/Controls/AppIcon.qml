// -----------------------------------------------------------------------
// File: AppIcon.qml
// Description: Tintable SVG icon primitive
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
//
// Every icon in the application goes through this one component. The icon
// SVGs are authored as white strokes on transparency, so tinting is a pure
// alpha multiply - no shader pass, no runtime SVG re-parse, and it stays
// cheap on OpenGL ES 2.0 hardware.
//
// ColorImage lives in QtQuick.Controls.impl. It is the same primitive Qt
// Quick Controls uses for its own icons, so it is well exercised, but it
// is not public API. Everything that draws an icon depends on AppIcon and
// nothing depends on ColorImage directly, so swapping the implementation
// is a single-file change if a future Qt release moves it.
//
// sourceSize is pinned to the drawn size so the SVG rasterises at exactly
// the pixels it occupies. Leaving it unset makes Qt render at the SVG's
// intrinsic 24x24 and then scale the texture, which is visibly soft.
//
import QtQuick
import QtQuick.Controls.impl
import "../Theme"

ColorImage {
    id: icon

    /** Registry name, e.g. "nav-alarms". Overridden by an explicit source. */
    property string name: ""

    /** Drawn edge length in pixels. Square by contract. */
    property int size: Math.round(24 * Metrics.scale)

    source: icon.name.length > 0 ? Icons.get(icon.name) : ""
    color: Colors.textPrimary

    // ColorImage derives from QQuickImplicitSizeItem, which exposes
    // implicitWidth/implicitHeight as read-only to QML. Setting sourceSize is
    // the right lever anyway: it both rasterises the SVG at exactly the pixels
    // it occupies - leaving it unset renders at the intrinsic 24x24 and scales
    // the texture, which is visibly soft - and gives the item its implicit
    // size for free, so it lays out correctly inside a Layout.
    width: icon.size
    height: icon.size
    sourceSize.width: icon.size
    sourceSize.height: icon.size

    fillMode: Image.PreserveAspectFit
    smooth: true
    mipmap: false
    asynchronous: false
    cache: true
}
