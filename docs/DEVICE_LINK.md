# Device link and clinical flow

## Two processors

The ventilator is two computers. One delivers the breath and owns every
measurement; the other draws this interface. They are joined by a bus, and
the split is what makes the behaviour below the only correct behaviour:

- The device is the authority on what is being delivered. The interface
  displays; it does not decide.
- The device keeps ventilating when the interface restarts. A restarted
  screen must never command a stop, and must never push its freshly
  initialised setpoints at a running therapy.
- The interface must be able to tell a quiet bus from a quiet patient. That
  is the heartbeat watchdog.

## Frames

`src/transport/data/ventilator.dbc` is the contract. Both sides read it.

| Identifier | Name | Direction | Rate | Carries |
| --- | --- | --- | --- | --- |
| 0x120 | VentWaveform | device to interface | 50 Hz | airway pressure, flow, volume, carbon dioxide |
| 0x121 | VentPressures | device to interface | 4 Hz | peak, plateau, mean, total PEEP |
| 0x122 | VentVolumes | device to interface | 4 Hz | expired tidal volume, minute volume, rate, leak, compliance |
| 0x123 | VentGas | device to interface | 4 Hz | oxygen, saturation, end-tidal carbon dioxide, resistance |
| 0x124 | VentStatus | device to interface | 4 Hz | battery, fault bit field, device state |
| 0x200 | VentSetpoints | interface to device | on change | oxygen, PEEP, rate, tidal volume, pressure support, inspiratory time, trigger |
| 0x201 | VentCommand | interface to device | on action | start, stop, holds, manual breath, oxygen boost, nebuliser, alarm reset |

Setpoints are sent as a whole frame every time one of them changes. Sending
only the field that moved would let the two sides drift apart after a single
dropped frame.

## Running it on a desk

The device speaks CAN. Development machines usually have no CAN interface and
macOS has none at all, so the same frames travel as loopback datagrams: four
bytes of identifier, one of length, then the payload. Decoding, the heartbeat,
the setpoint path and the adopt-on-reconnect logic are the code the device
runs; only the wire underneath changes.

Terminal one:

    python3 tools/sim/ventilator_sim.py

Terminal two: just run the application. The loopback link is the default,
so there is nothing to set. With no simulator answering the link never comes
up and the internal model runs, which is the ordinary desktop case.

    ./MedicalProject

In Qt Creator, if you do want to force a mode, the variable belongs in
**Projects, Run, Environment** - not Build Environment. The build environment
is the compiler's, and the application never sees it.

The simulator takes commands while it runs. `help` lists them. The useful
ones for exercising the interface:

    set spo2 84          force a saturation the low SpO2 alarm will catch
    fault occlusion      raise the occlusion bit, which drives peak pressure up
    fault disconnect     stop delivering, leak goes to 85 percent
    apnea 30             no breaths for thirty seconds
    battery 15           drive the battery alarm
    clear all            back to a healthy circuit
    show                 current sensor values and the setpoints received

Change a dial in the interface and the simulator prints the setpoint frame it
received. Press Start and it prints the command. That is the loop closed in
both directions.

Environment:

| Variable | Meaning |
| --- | --- |
| `SV_TELEMETRY` | `simulator` for the internal model with no link at all, `can` for a real bus. Absent, or anything else, opens the loopback link. |
| `SV_UDP_LISTEN_PORT` | Interface listen port, 35200 by default |
| `SV_UDP_SEND_PORT` | Interface send port, 35201 by default |
| `SV_CAN_PLUGIN` | Qt SerialBus plugin, `socketcan` on the target |
| `SV_CAN_INTERFACE` | Interface name, `can0` on the target |

Moving to real CAN on the target is `SV_TELEMETRY=can SV_CAN_PLUGIN=socketcan
SV_CAN_INTERFACE=can0`, with Qt SerialBus present in the kit. Nothing above
the transport layer changes.

## Clinical flow

Standby is the entry state, and the start is gated the way a real device
gates it:

1. **Admit a patient.** The category decides every safe range. A category
   carried over from the previous patient is a hazard, so admitting a patient
   also invalidates the previous pre-use check: the circuit was changed.
2. **Run the pre-use check.** Tests and Calibration runs the tightness test,
   the flow sensor zero and the oxygen cell calibration. Tightness and flow
   need standby, because a leak cannot be measured against a moving circuit
   pressure. All three passing is what sets the pre-use check.
3. **Start.** The button is disabled until both are true, and the reason it is
   disabled is shown under it rather than left for the operator to guess.
   `overridePreUseCheck(reason)` exists for an emergency and demands a reason,
   which goes to the audit trail - a device that can be started unchecked
   silently is a device that is always started unchecked.

Start and Stop are on the control rail, which is on every screen, so standby
is always one press away.

## After a crash

Two cases, and they are different.

**The interface restarts with a device attached.** The device keeps
ventilating throughout; it never sees the restart. On reconnect the interface
adopts what the device reports on 0x124: if the device is ventilating, the
interface marks the patient admitted and the pre-use check passed, because
both were satisfied before the crash, and shows the therapy in progress. It
does not send setpoints on reconnect, and the running-state change from
adopting is suppressed so it is not echoed back as a command.

**The interface restarts with no device.** There is nothing to ask, so the
bedside session written at every admit, discharge, start and stop is reloaded:
the patient, the category, the weight and the pre-use check come back. A crash
does not present an empty standby screen while a patient is still on the
circuit.

Either way the event is written to the log, so the gap is visible afterwards.

## Screen lock

The bedside account is `clinician`, number `0000`, created on first run when
no account exists and recorded as a default that must be replaced before
clinical use. `SMARTVENT_ADMIN_USER` and `SMARTVENT_ADMIN_PIN` provision a
real account instead. The lock button sits beside the clock; the inactivity
timeout is 300 seconds.
