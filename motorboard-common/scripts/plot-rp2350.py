#!/usr/bin/env python3
"""Live plot of RP2350 SPI status output.

Usage: python plot_spi.py /dev/ttyACM0   (or COM5 on Windows)
Deps:  pip install pyserial matplotlib
"""
import re
import sys
import threading
import time
from collections import deque

import matplotlib.pyplot as plt
import matplotlib.animation as animation
import serial

PORT = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0"
BAUD = 115200          # ignored for USB CDC, matters for UART
WINDOW = 1000          # samples shown
RATE_PERIOD = 0.25     # s, rate measurement interval
RATE_HISTORY = 240     # rate points shown (~60 s)
RATE_YMAX = 1500       # fixed y-axis max for samples/s

LINE_RE = re.compile(
    r"\[(\d+)\]\s+vel=(-?\d+)\s+vel_sp=(-?\d+)\s+cur=(-?\d+)\s+mA\s+"
    r"cur_sp=(-?\d+)\s+mA\s+duty=(-?\d+)\s+dir=(-?\d+)\s+coast=(-?\d+)"
)

FIELDS = ["n", "vel", "vel_sp", "cur", "cur_sp", "duty", "dir", "coast"]
data = {k: deque(maxlen=WINDOW) for k in FIELDS}
rate_t = deque(maxlen=RATE_HISTORY)   # seconds since start
rate_host = deque(maxlen=RATE_HISTORY)  # lines parsed per second
rate_fw = deque(maxlen=RATE_HISTORY)    # firmware frames per second (from n)
lock = threading.Lock()


def reader():
    ser = serial.Serial(PORT, BAUD, timeout=1)
    t0 = time.monotonic()
    win_start = t0
    win_count = 0
    win_first_n = None
    last_n = None
    while True:
        line = ser.readline().decode(errors="ignore").strip()
        m = LINE_RE.search(line)
        if not m:
            continue
        vals = [int(v) for v in m.groups()]
        now = time.monotonic()
        if win_first_n is None:
            win_first_n = vals[0]
        last_n = vals[0]
        win_count += 1
        with lock:
            if (vals[1] < -200000 or vals[1] > 200000):
                # throw out the value
                continue

            if (vals[5] < -1000 or vals[5] > 1000):
                # throw out the value
                continue

            if (vals[6] < -1 or vals[6] > 1):
                # throw out the value
                continue

            if (vals[7] < -1 or vals[7] > 1):
                # throw out the value
                continue

            for k, v in zip(FIELDS, vals):
                data[k].append(v)
            dt = now - win_start
            if dt >= RATE_PERIOD:
                rate_t.append(now - t0)
                rate_host.append(win_count / dt)
                # frames the firmware produced in this window (includes any lost)
                rate_fw.append((last_n - win_first_n + 1) / dt)
                win_start = now
                win_count = 0
                win_first_n = None


threading.Thread(target=reader, daemon=True).start()

fig, axes = plt.subplots(5, 1, figsize=(10, 10))
ax_vel, ax_cur, ax_duty, ax_state, ax_rate = axes
for ax in (ax_cur, ax_duty, ax_state):
    ax.sharex(ax_vel)
for ax in (ax_vel, ax_cur, ax_duty):
    ax.tick_params(labelbottom=False)

(l_vel,) = ax_vel.plot([], [], label="velocity")
(l_vsp,) = ax_vel.plot([], [], "--", label="setpoint")
(l_cur,) = ax_cur.plot([], [], label="current")
(l_csp,) = ax_cur.plot([], [], "--", label="setpoint")
(l_duty,) = ax_duty.plot([], [], label="duty")
(l_dir,) = ax_state.plot([], [], drawstyle="steps-post", label="direction")
(l_coast,) = ax_state.plot([], [], drawstyle="steps-post", label="coast")
(l_rate,) = ax_rate.plot([], [], label="received")
(l_rfw,) = ax_rate.plot([], [], "--", label="firmware (from n)")

ax_vel.set_ylabel("velocity")
ax_cur.set_ylabel("current (mA)")
ax_duty.set_ylabel("duty (m)")
ax_state.set_ylabel("state")
ax_state.set_xlabel("frame")
ax_rate.set_ylabel("samples/s")
ax_rate.set_ylim(0, RATE_YMAX)
ax_rate.set_xlabel("time (s)")
for ax in axes:
    ax.grid(True, alpha=0.3)
    ax.legend(loc="upper left")

pairs = [
    (ax_vel, [(l_vel, "vel"), (l_vsp, "vel_sp")]),
    (ax_cur, [(l_cur, "cur"), (l_csp, "cur_sp")]),
    (ax_duty, [(l_duty, "duty")]),
    (ax_state, [(l_dir, "dir"), (l_coast, "coast")]),
]


def update(_):
    with lock:
        x = list(data["n"])
        ys = {k: list(data[k]) for k in FIELDS[1:]}
        rt = list(rate_t)
        rh = list(rate_host)
        rf = list(rate_fw)
    artists = []
    if len(x) >= 2:
        for ax, lines in pairs:
            for line, key in lines:
                line.set_data(x, ys[key])
                artists.append(line)
            ax.set_xlim(x[0], x[-1])
            ax.relim()
            ax.autoscale_view(scalex=False)
    if len(rt) >= 2:
        l_rate.set_data(rt, rh)
        l_rfw.set_data(rt, rf)
        ax_rate.set_xlim(rt[0], rt[-1])
        ax_rate.set_title(
            f"{rh[-1]:.0f} samples/s received", fontsize=9, loc="right")
        artists += [l_rate, l_rfw]
    return artists


ani = animation.FuncAnimation(
    fig, update, interval=50, blit=False, cache_frame_data=False)
plt.tight_layout()
plt.show()
