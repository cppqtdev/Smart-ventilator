#!/usr/bin/env python3
"""Generate the alarm burst files in ui/Assets/ from their description.

The files are generated rather than recorded so that the parameters of every
tone are written down, reviewable and reproducible, which is what a design
history file needs. Run this script to rebuild them; do not edit the .wav
files by hand.

IEC 60601-1-8 Table 3 and Table 4 give the characteristics of an auditory
alarm signal:

  - fundamental between 150 Hz and 1000 Hz
  - at least four harmonics between 300 Hz and 4000 Hz
  - pulse duration 75 ms to 200 ms
  - rise time 10 % to 20 % of the pulse duration
  - high priority: a burst of ten pulses, as five, a gap, then five
  - medium priority: a burst of three pulses
  - low priority: a burst of one or two pulses

The values chosen here sit inside those windows. The burst interval - how
often the burst repeats - is applied by AlarmAudio, not baked into the file.

NOT YET VERIFIED against the published standard text: the exact inter-pulse
spacings within a high priority burst. The spacings below follow the widely
published pattern and are inside the permitted range, but they must be
confirmed against IEC 60601-1-8 Table 4 before the device is submitted.
"""

import math
import os
import struct
import wave

RATE = 44100
FUNDAMENTAL = 440.0          # Hz, inside the 150-1000 Hz window
HARMONIC_GAINS = [1.0, 0.55, 0.32, 0.18, 0.10]   # f, 2f, 3f, 4f, 5f
PULSE_MS = 150               # inside the 75-200 ms window
RISE_FRACTION = 0.15         # inside the 10-20 % window
PEAK = 0.55                  # leaves headroom so the sum never clips

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "..", "ui", "Assets")


def pulse(duration_ms=PULSE_MS, gain=1.0):
    """One alarm pulse: the fundamental with four harmonics over it."""
    count = int(RATE * duration_ms / 1000.0)
    rise = max(1, int(count * RISE_FRACTION))
    out = []
    for n in range(count):
        value = 0.0
        for index, weight in enumerate(HARMONIC_GAINS):
            frequency = FUNDAMENTAL * (index + 1)
            if frequency > 4000.0:
                break
            value += weight * math.sin(2.0 * math.pi * frequency * n / RATE)
        value /= sum(HARMONIC_GAINS)

        # Raised cosine on the way in and on the way out, so the pulse has no
        # click at either end.
        if n < rise:
            value *= 0.5 - 0.5 * math.cos(math.pi * n / rise)
        elif n > count - rise:
            value *= 0.5 - 0.5 * math.cos(math.pi * (count - n) / rise)

        out.append(value * PEAK * gain)
    return out


def silence(duration_ms):
    return [0.0] * int(RATE * duration_ms / 1000.0)


def burst(spacings_ms, gains=None):
    """Pulses separated by the given gaps. One more pulse than gaps."""
    gains = gains or [1.0] * (len(spacings_ms) + 1)
    samples = []
    for index, gain in enumerate(gains):
        samples += pulse(gain=gain)
        if index < len(spacings_ms):
            samples += silence(spacings_ms[index])
    return samples


def write(name, samples):
    path = os.path.join(OUT, name)
    os.makedirs(OUT, exist_ok=True)
    with wave.open(path, "wb") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(RATE)
        frames = b"".join(
            struct.pack("<h", int(max(-1.0, min(1.0, value)) * 32767))
            for value in samples
        )
        handle.writeframes(frames)
    seconds = len(samples) / float(RATE)
    print("%-20s %6.2f s" % (name, seconds))


def main():
    # High priority: five pulses, a gap, five pulses. The third gap is wider,
    # which is what gives the burst its recognisable rhythm.
    high = burst([100, 100, 200, 100, 350, 100, 100, 200, 100],
                 gains=[1.0, 1.0, 1.0, 0.9, 0.9,
                        1.0, 1.0, 1.0, 0.9, 0.9])
    write("alarm_high.wav", high)

    # Medium priority: three pulses.
    write("alarm_medium.wav", burst([150, 150]))

    # Low priority: two pulses.
    write("alarm_low.wav", burst([150], gains=[0.85, 0.85]))

    # One pulse, for the power-on speaker check and for a key press that has
    # to be heard.
    write("alarm_test.wav", pulse())


if __name__ == "__main__":
    main()
