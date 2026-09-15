import QtQuick
import QtTest
import "qrc:/ui/Components"
import "qrc:/ui/Theme"
import "qrc:/ui/Screens/Home"

TestCase {
    id: suite
    name: "Components"
    width: 1200
    height: 800
    when: windowShown

    Component {
        id: tileComponent
        MetricTile { width: 220; height: 110 }
    }

    Component {
        id: bannerComponent
        AlarmBanner { width: 700; height: 70 }
    }

    Component {
        id: readoutComponent
        NumericReadout { width: 120 }
    }

    Component {
        id: shellComponent
        ScreenShell { width: 1200; height: 800 }
    }

    Component {
        id: spyComponent
        SignalSpy {}
    }

    function test_tile_shows_every_limit_spelling_data() {
        return [
            { tag: "upperLimit/lowerLimit", props: { upperLimit: "40", lowerLimit: "5" } },
            { tag: "highValue/lowValue",    props: { highValue: 40, lowValue: 5 } },
            { tag: "highLimit/lowLimit",    props: { highLimit: 40, lowLimit: 5 } }
        ]
    }

    function test_tile_shows_every_limit_spelling(row) {
        var tile = createTemporaryObject(tileComponent, suite, row.props)
        verify(tile, "MetricTile failed to load with " + row.tag)
        compare(tile.upperText, "40")
        compare(tile.lowerText, "5")
    }

    function test_tile_prefers_the_explicit_string() {
        var tile = createTemporaryObject(tileComponent, suite,
                                         { upperLimit: "99", highValue: 40 })
        compare(tile.upperText, "99")
    }

    function test_tile_with_no_limits_shows_nothing() {
        var tile = createTemporaryObject(tileComponent, suite)
        compare(tile.upperText, "")
        compare(tile.lowerText, "")
    }

    function test_tile_priority_marks_the_alarm() {
        var tile = createTemporaryObject(tileComponent, suite, { priority: 3 })
        verify(tile.critical)
        verify(Qt.colorEqual(tile.alarmTint, Colors.alarmHigh))

        var medium = createTemporaryObject(tileComponent, suite, { priority: 2 })
        verify(medium.cautionary)
        verify(Qt.colorEqual(medium.alarmTint, Colors.alarmMedium))
    }

    function test_tile_state_string_still_works() {
        var tile = createTemporaryObject(tileComponent, suite, { state: "critical" })
        verify(tile.critical)
    }

    function test_tile_reports_activation() {
        var tile = createTemporaryObject(tileComponent, suite)
        var spy = spyComponent.createObject(suite, { target: tile, signalName: "activated" })
        mouseClick(tile)
        compare(spy.count, 1)
    }

    function test_tile_dims_when_unavailable() {
        var tile = createTemporaryObject(tileComponent, suite, { available: false })
        verify(tile.opacity < 1.0)
    }

    function test_banner_maps_priority_to_the_iec_colours() {
        var banner = createTemporaryObject(bannerComponent, suite)
        verify(Qt.colorEqual(banner.surfaceFor(3), Colors.alarmHigh))
        verify(Qt.colorEqual(banner.surfaceFor(2), Colors.alarmMedium))
        verify(Qt.colorEqual(banner.surfaceFor(1), Colors.alarmLow))
        verify(Qt.colorEqual(banner.surfaceFor(0), Colors.alarmNone))
    }

    function test_banner_shows_at_most_two_rows() {
        var banner = createTemporaryObject(bannerComponent, suite, {
            alarms: [
                { priority: 3, text: "High Minute Volume" },
                { priority: 2, text: "CT Low" },
                { priority: 1, text: "Third" }
            ]
        })
        compare(banner.visibleAlarms.length, 2)
        compare(banner.visibleAlarms[0].text, "High Minute Volume")
    }

    function test_banner_with_no_alarms_is_empty() {
        var banner = createTemporaryObject(bannerComponent, suite, { alarms: [] })
        compare(banner.visibleAlarms.length, 0)
    }

    function test_readout_hides_an_empty_unit() {
        var readout = createTemporaryObject(readoutComponent, suite,
                                            { label: "Pcuff", value: "20", unit: "" })
        verify(readout)
        compare(readout.unit, "")
    }

    function test_shell_maps_every_destination_to_its_tab() {
        var shell = createTemporaryObject(shellComponent, suite)
        var keys = ["monitoring", "controls", "system", "layout",
                    "events", "alarms", "tools", "modes"]
        for (var i = 0; i < keys.length; ++i)
            compare(shell.tabIndexFor(keys[i]), i, keys[i] + " maps to the wrong tab")
        compare(shell.tabIndexFor("nonsense"), 0, "an unknown screen falls back to the first tab")
    }

    function test_shell_has_the_eight_reference_tabs() {
        var shell = createTemporaryObject(shellComponent, suite)
        compare(shell.navigationTabs.length, 8)
    }

    function test_home_layouts_resolve_to_a_real_file() {
        compare(HomeLayouts.entries.length, 5)
        for (var i = 0; i < HomeLayouts.entries.length; ++i) {
            var entry = HomeLayouts.entries[i]
            var source = HomeLayouts.sourceFor(entry.id)
            verify(source.indexOf("qrc:/ui/Screens/Home/") === 0,
                   entry.label + " resolves to '" + source + "'")
            verify(source.indexOf(".qml") > 0)
            verify(entry.cells.length > 0)
        }
    }

    function test_home_layouts_unknown_id_falls_back() {
        var entry = HomeLayouts.entryFor(999)
        compare(entry.id, HomeLayouts.entries[0].id)
    }
}
