// -----------------------------------------------------------------------
// File: AppTabButton.qml
// Description: Checkable tab used by AppTabBar and the sub-tab strips
// Part of: Smart Ventilator and Respiratory Monitoring UI
// -----------------------------------------------------------------------
import QtQuick
import "../Theme"

AppButton {
    id: tab

    property bool useSuccessPalette: false

    buttonVariant: tab.useSuccessPalette ? AppButton.Success
                                   : AppButton.Primary
    checkable: true
    radius: Radius.small
    fontSize: Typography.tabLabel
    fontFamily: Typography.monoFamily
    fontWeight: Typography.bold
    implicitHeight: Metrics.navHeight

    Accessible.role: Accessible.PageTab
    Accessible.name: tab.text
    Accessible.checkable: true
    Accessible.checked: tab.checked
}
