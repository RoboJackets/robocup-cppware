#!/usr/bin/env python3
"""Talk to the serial device on /dev/ttyACM0, run it N times, and plot all runs.

Protocol:
  host sends any byte ->
  TEST START / TEST END / DATA START / "index,value" x N / DATA END
"""
import argparse
import sys

import matplotlib.pyplot as plt
import numpy as np
import serial  # pip install pyserial
import time


def read_line(ser):
    """Read one stripped text line; raise on timeout."""
    raw = ser.readline()
    if not raw:
        raise TimeoutError("Timed out waiting for device response")
    return raw.decode("ascii", errors="replace").strip()


def expect(ser, marker):
    """Consume lines until the given marker line is seen."""
    while True:
        line = read_line(ser)
        if line == marker:
            return
        if line:
            print(
                f"(ignored while waiting for {marker!r}): {line}", file=sys.stderr)


def run_transaction(ser, trigger=b"\n"):
    ser.reset_input_buffer()
    ser.write(trigger)  # any byte starts the sequence
    ser.flush()

    expect(ser, "TEST START")
    expect(ser, "TEST END")
    expect(ser, "DATA START")

    indices, values = [], []
    while True:
        line = read_line(ser)
        if line == "DATA END":
            break
        if not line:
            continue
        try:
            idx, val = line.split(",")
            indices.append(int(idx))
            values.append(float(val))
        except ValueError:
            print(f"Skipping malformed line: {line!r}", file=sys.stderr)
    return np.array(indices), np.array(values)


def smooth(values, window):
    """Moving average with edge padding so output length matches input."""
    if window <= 1 or len(values) < window:
        return values
    pad = window // 2
    padded = np.pad(values, (pad, window - 1 - pad), mode="edge")
    return np.convolve(padded, np.ones(window) / window, mode="valid")


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--port", default="/dev/ttyACM0")
    p.add_argument("--baud", type=int, default=115200)
    p.add_argument("--timeout", type=float, default=5.0)
    p.add_argument("--runs", type=int, default=10)
    p.add_argument("--window", type=int, default=9,
                   help="smoothing window (samples); 1 = off")
    p.add_argument("--dt", type=float, default=1, help="ms per index")
    p.add_argument("--scale", type=float, default=20.0,
                   help="raw value per rotation/s")
    args = p.parse_args()

    # Open the window before the first run and update after each one
    plt.ion()
    fig, ax = plt.subplots()
    ax.set_xlabel("Time (ms)")
    ax.set_ylabel("Rotations per second")

    # setpoint line
    ax.axhline(50, color="red", linestyle="--", linewidth=1)

    ax.set_title(f"Device data ({args.window}-pt moving avg)")
    ax.grid(True)
    fig.tight_layout()
    fig.canvas.draw()
    plt.pause(0.05)

    with serial.Serial(args.port, args.baud, timeout=args.timeout) as ser:
        for i in range(1, args.runs + 1):
            idx, val = run_transaction(ser)
            print(f"Run {i}/{args.runs}: {len(val)} samples")
            ax.plot(idx * args.dt, smooth(val / args.scale,
                    args.window), linewidth=1.2, label=f"Run {i}")
            ax.relim()
            ax.autoscale_view()
            ax.legend(ncol=2, fontsize="small")
            fig.canvas.draw_idle()
            plt.pause(0.05)
            time.sleep(1)

    ax.set_title(
        f"Device data ({args.runs} runs, {args.window}-pt moving avg)")
    plt.ioff()
    plt.show()  # keep window open until closed


if __name__ == "__main__":
    main()
