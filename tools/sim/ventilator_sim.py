#!/usr/bin/env python3
"""Stand-in for the ventilator hardware.

Speaks the frames in src/transport/data/ventilator.dbc: publishes the sensor
frames the interface reads, and accepts the setpoint and command frames the
interface writes. The interface cannot tell this apart from the device, which
is the point - the same decode, the same heartbeat, the same setpoint path.

Transport
---------
The device uses CAN. Development machines usually have no CAN interface, and
macOS has none at all, so the same frames travel as UDP datagrams on
loopback. The datagram is the frame and nothing else:

    [0..3] frame identifier, little endian
    [4]    payload length, 0 to 8
    [5..]  payload

Swapping this for python-can on the target is a change of two functions.

Usage
-----
    python3 tools/sim/ventilator_sim.py

    SV_TELEMETRY=udp ./MedicalProject      # in the other terminal

Type commands at the prompt while it runs:

    help                 list the commands
    show                 current sensor values and setpoints
    set <name> <value>   force a sensor value, e.g. set spo2 84
    auto <name>          hand a forced value back to the model
    fault <name>         raise a device fault bit, e.g. fault occlusion
    clear <name>         clear one fault, or 'clear all'
    apnea <seconds>      stop delivering breaths for a while
    battery <percent>    set the battery reading
    quit
"""

from __future__ import annotations

import math
import os
import queue
import socket
import struct
import sys
import threading
import time
from dataclasses import dataclass, field

# --------------------------------------------------------------------------
# Frame definitions. These mirror src/transport/data/ventilator.dbc exactly;
# if you change one, change both.
# --------------------------------------------------------------------------

INTEL = True


@dataclass(frozen=True)
class Signal:
    name: str
    start_bit: int
    bit_length: int
    signed: bool
    factor: float
    offset: float
    minimum: float
    maximum: float


@dataclass(frozen=True)
class Message:
    frame_id: int
    name: str
    length: int
    signals: tuple


WAVEFORM = Message(0x120, "VentWaveform", 8, (
    Signal("airwayPressure", 0, 16, True, 0.01, 0.0, -20.0, 120.0),
    Signal("flow", 16, 16, True, 0.1, 0.0, -300.0, 300.0),
    Signal("volume", 32, 16, False, 0.5, 0.0, 0.0, 3000.0),
    Signal("co2", 48, 16, False, 0.01, 0.0, 0.0, 100.0),
))

PRESSURES = Message(0x121, "VentPressures", 8, (
    Signal("peakPressure", 0, 16, False, 0.1, 0.0, 0.0, 120.0),
    Signal("plateauPressure", 16, 16, False, 0.1, 0.0, 0.0, 120.0),
    Signal("meanPressure", 32, 16, False, 0.1, 0.0, 0.0, 120.0),
    Signal("peep", 48, 16, False, 0.1, 0.0, 0.0, 60.0),
))

VOLUMES = Message(0x122, "VentVolumes", 8, (
    Signal("tidalVolumeExpired", 0, 16, False, 0.5, 0.0, 0.0, 3000.0),
    Signal("minuteVolume", 16, 16, False, 0.01, 0.0, 0.0, 60.0),
    Signal("respiratoryRate", 32, 8, False, 0.5, 0.0, 0.0, 120.0),
    Signal("leakPercent", 40, 8, False, 0.5, 0.0, 0.0, 100.0),
    Signal("compliance", 48, 16, False, 0.1, 0.0, 0.0, 300.0),
))

GAS = Message(0x123, "VentGas", 8, (
    Signal("fio2", 0, 16, False, 0.1, 0.0, 0.0, 100.0),
    Signal("spo2", 16, 8, False, 1.0, 0.0, 0.0, 100.0),
    Signal("etco2", 24, 16, False, 0.1, 0.0, 0.0, 150.0),
    Signal("resistance", 40, 16, False, 0.1, 0.0, 0.0, 200.0),
))

STATUS = Message(0x124, "VentStatus", 8, (
    Signal("batteryPercent", 0, 8, False, 1.0, 0.0, 0.0, 100.0),
    Signal("deviceFault", 8, 32, False, 1.0, 0.0, 0.0, 4294967295.0),
    Signal("deviceState", 40, 8, False, 1.0, 0.0, 0.0, 255.0),
))

SETPOINTS = Message(0x200, "VentSetpoints", 8, (
    Signal("setFio2", 0, 8, False, 1.0, 0.0, 21.0, 100.0),
    Signal("setPeep", 8, 8, False, 1.0, 0.0, 0.0, 50.0),
    Signal("setRate", 16, 8, False, 1.0, 0.0, 0.0, 120.0),
    Signal("setTidalVolume", 24, 16, False, 1.0, 0.0, 0.0, 3000.0),
    Signal("setPressureSupport", 40, 8, False, 1.0, 0.0, 0.0, 60.0),
    Signal("setInspiratoryTime", 48, 8, False, 0.1, 0.0, 0.0, 10.0),
    Signal("setTrigger", 56, 8, False, 0.5, 0.0, 0.0, 40.0),
))

COMMAND = Message(0x201, "VentCommand", 8, (
    Signal("command", 0, 8, False, 1.0, 0.0, 0.0, 255.0),
))

INBOUND = {message.frame_id: message for message in (SETPOINTS, COMMAND)}

COMMAND_NAMES = {
    0: "none",
    1: "start ventilation",
    2: "stop ventilation",
    3: "inspiratory hold",
    4: "expiratory hold",
    5: "manual breath",
    6: "oxygen boost",
    7: "nebuliser",
    8: "alarm reset",
}

# deviceFault is a bit field. The interface maps each bit to a technical
# alarm condition, so the names here are the contract, not decoration.
FAULT_BITS = {
    "occlusion": 1 << 0,
    "disconnect": 1 << 1,
    "o2supply": 1 << 2,
    "airsupply": 1 << 3,
    "flowsensor": 1 << 4,
    "o2cell": 1 << 5,
    "battery": 1 << 6,
    "fan": 1 << 7,
}


def encode(message: Message, values: dict) -> bytes:
    """Packs physical values into the frame payload, little endian."""
    raw = 0
    for entry in message.signals:
        physical = values.get(entry.name)
        if physical is None:
            continue
        physical = max(entry.minimum, min(entry.maximum, float(physical)))
        number = int(round((physical - entry.offset) / entry.factor))
        if entry.signed:
            limit = 1 << (entry.bit_length - 1)
            number = max(-limit, min(limit - 1, number))
            if number < 0:
                number += 1 << entry.bit_length
        else:
            number = max(0, min((1 << entry.bit_length) - 1, number))
        raw |= (number & ((1 << entry.bit_length) - 1)) << entry.start_bit
    return raw.to_bytes(message.length, "little")


def decode(message: Message, payload: bytes) -> dict:
    raw = int.from_bytes(payload.ljust(message.length, b"\x00"), "little")
    out = {}
    for entry in message.signals:
        number = (raw >> entry.start_bit) & ((1 << entry.bit_length) - 1)
        if entry.signed and number >= (1 << (entry.bit_length - 1)):
            number -= 1 << entry.bit_length
        out[entry.name] = number * entry.factor + entry.offset
    return out


# --------------------------------------------------------------------------
# The device model
# --------------------------------------------------------------------------

@dataclass
class Setpoints:
    fio2: float = 40.0
    peep: float = 5.0
    rate: float = 14.0
    tidal_volume: float = 500.0
    pressure_support: float = 12.0
    inspiratory_time: float = 1.2
    trigger: float = 3.0


@dataclass
class Device:
    setpoints: Setpoints = field(default_factory=Setpoints)
    ventilating: bool = False
    fault_bits: int = 0
    battery: float = 100.0
    apnea_until: float = 0.0
    forced: dict = field(default_factory=dict)

    phase: float = 0.0
    breath_count: int = 0

    # Per-breath results, recomputed at the end of each expiration.
    peak: float = 0.0
    plateau: float = 0.0
    mean: float = 0.0
    total_peep: float = 0.0
    vte: float = 0.0
    minute_volume: float = 0.0
    measured_rate: float = 0.0
    leak: float = 2.0
    compliance: float = 45.0
    resistance: float = 9.0
    spo2: float = 97.0
    etco2: float = 38.0

    def value(self, name: str, computed: float) -> float:
        """A forced value wins, so a fault can be injected from the prompt."""
        return self.forced.get(name, computed)

    def apnoeic(self, now: float) -> bool:
        return now < self.apnea_until

    def step(self, dt: float, now: float) -> dict:
        """Advances one sample and returns the instantaneous waveform point."""
        if not self.ventilating or self.apnoeic(now):
            self.phase = 0.0
            return {"airwayPressure": 0.0, "flow": 0.0, "volume": 0.0, "co2": 0.0}

        period = 60.0 / max(4.0, self.setpoints.rate)
        previous = self.phase
        self.phase = (self.phase + dt / period) % 1.0
        if self.phase < previous:
            self.finish_breath()

        inspiratory_fraction = min(0.8, self.setpoints.inspiratory_time / period)
        occluded = bool(self.fault_bits & FAULT_BITS["occlusion"])
        disconnected = bool(self.fault_bits & FAULT_BITS["disconnect"])

        drive = self.setpoints.peep + self.setpoints.pressure_support
        if occluded:
            drive += 25.0
        if disconnected:
            drive = 0.5

        if self.phase < inspiratory_fraction:
            shape = self.phase / max(1e-6, inspiratory_fraction)
            pressure = self.setpoints.peep + (drive - self.setpoints.peep) * (1 - math.exp(-4 * shape))
            flow = 40.0 * math.exp(-3 * shape)
            volume = self.setpoints.tidal_volume * shape
            co2 = 0.5
        else:
            shape = (self.phase - inspiratory_fraction) / max(1e-6, 1 - inspiratory_fraction)
            pressure = self.setpoints.peep + (drive - self.setpoints.peep) * math.exp(-5 * shape)
            flow = -45.0 * math.exp(-4 * shape)
            volume = self.setpoints.tidal_volume * math.exp(-3 * shape)
            co2 = self.etco2 * (1 - math.exp(-6 * shape))

        if disconnected:
            pressure, flow, volume, co2 = 0.2, 0.0, 0.0, 0.0

        return {
            "airwayPressure": self.value("airwayPressure", pressure),
            "flow": self.value("flow", flow),
            "volume": self.value("volume", volume),
            "co2": self.value("co2", co2),
        }

    def finish_breath(self) -> None:
        self.breath_count += 1
        occluded = bool(self.fault_bits & FAULT_BITS["occlusion"])
        disconnected = bool(self.fault_bits & FAULT_BITS["disconnect"])

        self.peak = self.setpoints.peep + self.setpoints.pressure_support + (25.0 if occluded else 0.0)
        self.plateau = self.peak - (2.0 if not occluded else 8.0)
        self.mean = self.setpoints.peep + (self.peak - self.setpoints.peep) * 0.35
        self.total_peep = self.setpoints.peep + (1.5 if occluded else 0.2)

        delivered = 0.0 if disconnected else self.setpoints.tidal_volume
        self.leak = 85.0 if disconnected else 2.0
        self.vte = delivered * (1 - self.leak / 100.0)
        self.measured_rate = self.setpoints.rate
        self.minute_volume = self.vte * self.measured_rate / 1000.0

        driving = max(1.0, self.plateau - self.total_peep)
        self.compliance = self.vte / driving if self.vte > 0 else 0.0
        self.resistance = 22.0 if occluded else 9.0

        if disconnected:
            self.spo2 = max(60.0, self.spo2 - 1.5)
            self.etco2 = max(2.0, self.etco2 - 3.0)
        else:
            self.spo2 = min(99.0, self.spo2 + 0.4)
            self.etco2 = min(45.0, self.etco2 + 0.6)

    def apply_setpoints(self, values: dict) -> None:
        self.setpoints.fio2 = values.get("setFio2", self.setpoints.fio2)
        self.setpoints.peep = values.get("setPeep", self.setpoints.peep)
        self.setpoints.rate = values.get("setRate", self.setpoints.rate)
        self.setpoints.tidal_volume = values.get("setTidalVolume", self.setpoints.tidal_volume)
        self.setpoints.pressure_support = values.get("setPressureSupport", self.setpoints.pressure_support)
        self.setpoints.inspiratory_time = values.get("setInspiratoryTime", self.setpoints.inspiratory_time)
        self.setpoints.trigger = values.get("setTrigger", self.setpoints.trigger)

    def apply_command(self, code: int) -> str:
        name = COMMAND_NAMES.get(code, f"unknown ({code})")
        if code == 1:
            self.ventilating = True
        elif code == 2:
            self.ventilating = False
        elif code == 5:
            self.phase = 0.0
        return name


# --------------------------------------------------------------------------
# Runner
# --------------------------------------------------------------------------

SAMPLE_HZ = 50
SLOW_HZ = 4


class Runner:
    def __init__(self, listen_port: int, send_port: int) -> None:
        self.device = Device()
        self.commands: queue.Queue = queue.Queue()
        self.running = True

        # The interface listens on listen_port and sends on send_port, so the
        # simulator is the mirror image of it.
        self.send_port = listen_port
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind(("127.0.0.1", send_port))
        self.socket.settimeout(0.005)

    def send(self, message: Message, values: dict) -> None:
        payload = encode(message, values)
        datagram = struct.pack("<IB", message.frame_id, len(payload)) + payload
        self.socket.sendto(datagram, ("127.0.0.1", self.send_port))

    def receive(self) -> None:
        while True:
            try:
                datagram, _ = self.socket.recvfrom(64)
            except (socket.timeout, BlockingIOError):
                return
            if len(datagram) < 5:
                continue
            frame_id, length = struct.unpack("<IB", datagram[:5])
            message = INBOUND.get(frame_id)
            if message is None:
                continue
            values = decode(message, datagram[5:5 + length])
            if frame_id == SETPOINTS.frame_id:
                self.device.apply_setpoints(values)
                print(f"\r  <- setpoints  fio2={values['setFio2']:.0f}%"
                      f" peep={values['setPeep']:.0f} rate={values['setRate']:.0f}"
                      f" vt={values['setTidalVolume']:.0f}mL", flush=True)
            else:
                name = self.device.apply_command(int(values["command"]))
                print(f"\r  <- command    {name}", flush=True)

    def publish_slow(self) -> None:
        device = self.device
        self.send(PRESSURES, {
            "peakPressure": device.value("peakPressure", device.peak),
            "plateauPressure": device.value("plateauPressure", device.plateau),
            "meanPressure": device.value("meanPressure", device.mean),
            "peep": device.value("peep", device.total_peep),
        })
        self.send(VOLUMES, {
            "tidalVolumeExpired": device.value("tidalVolumeExpired", device.vte),
            "minuteVolume": device.value("minuteVolume", device.minute_volume),
            "respiratoryRate": device.value("respiratoryRate", device.measured_rate),
            "leakPercent": device.value("leakPercent", device.leak),
            "compliance": device.value("compliance", device.compliance),
        })
        self.send(GAS, {
            "fio2": device.value("fio2", device.setpoints.fio2),
            "spo2": device.value("spo2", device.spo2),
            "etco2": device.value("etco2", device.etco2),
            "resistance": device.value("resistance", device.resistance),
        })
        self.send(STATUS, {
            "batteryPercent": device.value("batteryPercent", device.battery),
            "deviceFault": device.fault_bits,
            "deviceState": 1 if device.ventilating else 0,
        })

    def handle_prompt(self, line: str) -> None:
        device = self.device
        parts = line.split()
        if not parts:
            return
        verb = parts[0].lower()

        if verb in ("quit", "exit"):
            self.running = False
        elif verb == "help":
            print(__doc__.split("Type commands")[1])
        elif verb == "show":
            print(f"  ventilating : {device.ventilating}")
            print(f"  setpoints   : {device.setpoints}")
            print(f"  faults      : {self.fault_names() or 'none'}")
            print(f"  forced      : {device.forced or 'none'}")
            print(f"  battery     : {device.battery:.0f}%")
        elif verb == "set" and len(parts) == 3:
            device.forced[parts[1]] = float(parts[2])
            print(f"  {parts[1]} forced to {parts[2]}")
        elif verb == "auto" and len(parts) == 2:
            device.forced.pop(parts[1], None)
            print(f"  {parts[1]} back on the model")
        elif verb == "fault" and len(parts) == 2:
            bit = FAULT_BITS.get(parts[1].lower())
            if bit is None:
                print(f"  unknown fault, one of: {', '.join(FAULT_BITS)}")
            else:
                device.fault_bits |= bit
                print(f"  fault {parts[1]} raised")
        elif verb == "clear" and len(parts) == 2:
            if parts[1].lower() == "all":
                device.fault_bits = 0
                print("  all faults cleared")
            else:
                bit = FAULT_BITS.get(parts[1].lower())
                if bit is None:
                    print(f"  unknown fault, one of: {', '.join(FAULT_BITS)}")
                else:
                    device.fault_bits &= ~bit
                    print(f"  fault {parts[1]} cleared")
        elif verb == "apnea" and len(parts) == 2:
            device.apnea_until = time.time() + float(parts[1])
            print(f"  no breaths for {parts[1]} s")
        elif verb == "battery" and len(parts) == 2:
            device.battery = float(parts[1])
            print(f"  battery {parts[1]}%")
        else:
            print("  not understood, try 'help'")

    def fault_names(self) -> str:
        return ", ".join(name for name, bit in FAULT_BITS.items()
                         if self.device.fault_bits & bit)

    def prompt_loop(self) -> None:
        for line in sys.stdin:
            self.commands.put(line.strip())
            if not self.running:
                return

    def run(self) -> None:
        print(f"ventilator simulator: sending to 127.0.0.1:{self.send_port}, "
              f"listening on {self.socket.getsockname()[1]}")
        print("start the interface with SV_TELEMETRY=udp, then type 'help'")

        threading.Thread(target=self.prompt_loop, daemon=True).start()

        sample_period = 1.0 / SAMPLE_HZ
        slow_period = 1.0 / SLOW_HZ
        next_slow = time.time()
        last = time.time()

        while self.running:
            now = time.time()
            dt = now - last
            last = now

            self.receive()

            while not self.commands.empty():
                self.handle_prompt(self.commands.get())

            self.send(WAVEFORM, self.device.step(dt, now))

            if now >= next_slow:
                self.publish_slow()
                next_slow = now + slow_period

            time.sleep(max(0.0, sample_period - (time.time() - now)))

        self.socket.close()
        print("simulator stopped")


def main() -> int:
    listen_port = int(os.environ.get("SV_UDP_LISTEN_PORT", "35200"))
    send_port = int(os.environ.get("SV_UDP_SEND_PORT", "35201"))
    try:
        Runner(listen_port, send_port).run()
    except KeyboardInterrupt:
        print("\nsimulator stopped")
    return 0


if __name__ == "__main__":
    sys.exit(main())
