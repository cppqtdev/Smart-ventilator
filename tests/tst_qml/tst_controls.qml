import QtQuick
import QtTest
import "qrc:/ui/Controls"
import "qrc:/ui/Theme"

TestCase {
    id: suite
    name: "Controls"
    width: 800
    height: 600
    when: windowShown

    Component {
        id: buttonComponent
        AppButton { text: "Press" }
    }

    Component {
        id: tabBarComponent
        AppTabBar {
            model: [
                { key: "a", label: "Alpha" },
                { key: "b", label: "Beta" },
                { key: "c", label: "Gamma" }
            ]
        }
    }

    Component {
        id: dialComponent
        DialControl {
            width: 200
            label: "Oxygen"
            from: 21
            to: 100
            stepSize: 5
            value: 60
            unit: "%"
        }
    }

    Component {
        id: gaugeComponent
        RingGauge { from: 0; to: 200; value: 50 }
    }

    Component {
        id: checkComponent
        AppCheckBox { text: "Enabled" }
    }

    function test_button_reports_clicks() {
        var button = createTemporaryObject(buttonComponent, suite)
        verify(button)
        var spy = signalSpyComponent.createObject(suite, { target: button, signalName: "clicked" })
        mouseClick(button)
        compare(spy.count, 1)
    }

    function test_button_checkable_toggles() {
        var button = createTemporaryObject(buttonComponent, suite, { checkable: true })
        compare(button.checked, false)
        mouseClick(button)
        compare(button.checked, true)
        compare(button.active, true)
        mouseClick(button)
        compare(button.checked, false)
    }

    function test_disabled_button_swallows_clicks() {
        var button = createTemporaryObject(buttonComponent, suite,
                                           { checkable: true, enabled: false })
        mouseClick(button)
        compare(button.checked, false)
    }

    function test_button_variants_differ() {
        var primary = createTemporaryObject(buttonComponent, suite,
                                            { buttonVariant: AppButton.Primary })
        var danger = createTemporaryObject(buttonComponent, suite,
                                           { buttonVariant: AppButton.Danger })
        var ghost = createTemporaryObject(buttonComponent, suite,
                                          { buttonVariant: AppButton.Ghost })
        verify(!Qt.colorEqual(primary.baseColor, danger.baseColor))
        compare(ghost.baseColor.a, 0)
    }

    function test_button_meets_the_touch_floor() {
        var button = createTemporaryObject(buttonComponent, suite)
        verify(button.implicitHeight >= Metrics.touchMinimum)
    }

    function test_tabbar_selects_and_reports() {
        var bar = createTemporaryObject(tabBarComponent, suite, { width: 600, height: 48 })
        verify(bar)
        compare(bar.currentIndex, 0)
        compare(bar.labelAt(1), "Beta")
        compare(bar.keyAt(2), "c")

        var spy = signalSpyComponent.createObject(suite, { target: bar, signalName: "activated" })
        mouseClick(bar, bar.width * 0.5, bar.height * 0.5)
        compare(spy.count, 1)
        compare(bar.currentIndex, 1)
    }

    function test_tabbar_handles_an_empty_model() {
        var bar = createTemporaryObject(tabBarComponent, suite, { model: [] })
        compare(bar.labelAt(0), "")
        compare(bar.keyAt(0), "")
    }

    function test_dial_steps_within_range() {
        var dial = createTemporaryObject(dialComponent, suite)
        var spy = signalSpyComponent.createObject(suite, { target: dial, signalName: "stepRequested" })

        dial.propose(1)
        compare(spy.count, 1)
        compare(spy.signalArguments[0][0], 65)

        dial.propose(-1)
        compare(spy.count, 2)
        compare(spy.signalArguments[1][0], 55)
    }

    function test_dial_clamps_at_the_ends() {
        var dial = createTemporaryObject(dialComponent, suite, { value: 100 })
        var spy = signalSpyComponent.createObject(suite, { target: dial, signalName: "stepRequested" })
        dial.propose(1)
        compare(spy.count, 0, "stepping past the maximum must emit nothing")

        dial.value = 21
        dial.propose(-1)
        compare(spy.count, 0, "stepping past the minimum must emit nothing")
    }

    function test_dial_fits_the_width_it_is_given() {
        var widths = [120, 160, 200, 320, 480]
        for (var i = 0; i < widths.length; ++i) {
            var dial = createTemporaryObject(dialComponent, suite, { width: widths[i] })
            var total = dial.ringSize + dial.stepWidth * 2
            verify(total <= widths[i] + 1,
                   "dial of " + total + " px overflows a " + widths[i] + " px column")
            verify(dial.ringSize > 0)
        }
    }

    function test_gauge_fraction_tracks_the_value() {
        var gauge = createTemporaryObject(gaugeComponent, suite)
        compare(gauge.fraction, 0.25)
        gauge.value = 200
        compare(gauge.fraction, 1.0)
        gauge.value = -50
        compare(gauge.fraction, 0.0, "a value below the floor must not go negative")
    }

    function test_gauge_survives_a_degenerate_range() {
        var gauge = createTemporaryObject(gaugeComponent, suite, { from: 10, to: 10, value: 10 })
        verify(gauge.fraction >= 0 && gauge.fraction <= 1)
    }

    function test_checkbox_toggles() {
        var box = createTemporaryObject(checkComponent, suite)
        compare(box.checked, false)
        mouseClick(box)
        compare(box.checked, true)
    }

    Component {
        id: signalSpyComponent
        SignalSpy {}
    }
}
