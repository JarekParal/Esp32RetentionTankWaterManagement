# Esp32RetentionTankWaterManagement

ESP32 firmware for a residential water-retention tank controller running on the
**Hankerila HKL-EA8** board. Drives 8 solenoid valves, reads an HC-SR04
ultrasonic tank-level sensor, monitors 8 digital inputs (one of them a 1 L/pulse
water meter), shows live state on a small OLED, and exposes everything over
**HTTP** (browser UI) and **Modbus TCP/502** (PLC / HA / SCADA).

Wired Ethernet only — no Wi-Fi.

![Web UI](doc/web-ui-screenshot.png) <!-- TODO: add screenshot when convenient -->

---

## At a glance

| | |
| --- | --- |
| Board | Hankerila HKL-EA8 (ESP32-WROOM-32 + LAN8720 PHY) |
| Toolchain | `pioarduino/platform-espressif32@55.03.38` — Arduino core 3.3.8, ESP-IDF 5.5.4 |
| Network | Wired Ethernet, DHCP by default |
| Web UI | `http://<device-ip>/` |
| Modbus TCP | `<device-ip>:502` — see [doc/MODBUS.md](doc/MODBUS.md) |
| OTA | `pio run -e nodemcu-32s-ota -t upload` |
| Source layout | One Arduino sketch in `src/main.cpp`, helpers in `src/*.{h,cpp}` |

---

## What the device does

- **Valves**: 8 outputs on a PCF8574 I²C expander at `0x24`. Controlled from the Web UI, Modbus, or HTTP.
- **Inputs**: 8 inputs on a second PCF8574 at `0x26`, INT line wired to GPIO12 so polling is interrupt-driven, not timer-based.
- **Tank level**: HC-SR04 ultrasonic, sampled every 10 s; persisted as a 24 h × 10 min and 30 d × 1 day min/avg/max history.
- **Water meter**: Input 1 doubles as a 1 L/pulse counter. Per-minute flow rate is recorded into a second history ring identical in shape to the tank-distance one.
- **OLED**: 0.96" SSD1306 over the same I²C bus (`0x3C`) shows IP, uptime, tank cm + fill %, water L, valve+input mask, last log line.

---

## Hardware

**Board**: [Hankerila HKL-EA8](doc/Hankerila-EA8-board/) — vendor schematics + demo sketches live under `doc/Hankerila-EA8-board/`. When adding a new peripheral, copy the pin map and init pattern from the matching numbered demo folder rather than rediscovering it.

**Required HW modifications** for this firmware — see [doc/HW modification notes.md](<doc/HW modification notes.md>):

1. **Beeper → PCF8574 /INT to GPIO12** for interrupt-driven input polling.
2. **IR transmitter / receiver → HC-SR04** for tank level (R50 swap 2 kΩ → 200 Ω, R52 47 kΩ pull-up kept).

**Peripherals on the I²C bus** (SDA=GPIO4, SCL=GPIO5):

| Address | Device | Role |
| --- | --- | --- |
| `0x24` | PCF8574 | 8 relay outputs (active-low) |
| `0x26` | PCF8574 | 8 digital inputs (active-low, /INT → GPIO12) |
| `0x3C` | SSD1306 | 0.96" 128×64 OLED display |

**Other pins**:

| Function | GPIO |
| --- | --- |
| Ethernet MDC | 23 |
| Ethernet MDIO | 18 |
| Ethernet clock | 17 (out) |
| Ultrasonic TRIG | 32 |
| Ultrasonic ECHO | 33 |
| PCF8574 /INT in | 12 (`INPUT_PULLUP`) |

> ⚠️ GPIO12 is a strapping pin sampled at reset; never add an external pull-up — software `INPUT_PULLUP` activates after the strapping phase. A hardware pull-up switches the flash to 1.8 V mode and breaks flashing (MD5 mismatch).

---

## Build & flash

PlatformIO Core ≥ 6.1 required. The maintainer's machine doesn't have `pio` on PATH — invoke via the bundled `penv`:

```powershell
# Build (Windows / PowerShell)
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run

# USB flash
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t upload

# OTA flash (Ethernet), once a USB-flashed firmware is on the device
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e nodemcu-32s-ota -t upload

# Serial monitor @ 115200
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" device monitor
```

> ⚠️ **Run the first `pio run` from PowerShell or cmd, not Git Bash.** The `pioarduino` platform installer refuses MSys/Mingw environments and leaves `tool-esptoolpy` half-installed. After the first install completes, the Bash tool works fine for subsequent builds.

CI runs plain `pio run` on push/PR to `main` — see [.github/workflows/build.yml](.github/workflows/build.yml).

---

## Web UI (port 80)

Open `http://<device-ip>/`. Sections (foldable, state persists in localStorage):

| Section | Content |
| --- | --- |
| Tank Distance | 24 h (10-min buckets) and 30 d (daily) min/avg/max charts; "Clear 24h" / "Clear 30d" buttons with two-step confirm |
| Water Consumption | Same chart layout for L/min flow rate sampled every minute from the Input 1 pulse counter |
| Solenoid Valves | 8 toggle tiles + "Close all" |
| Digital Inputs | 8 status tiles (read-only) |
| Device Log | Live tail of the firmware's in-memory log buffer (60 lines) |

Header metrics: tank level (% or cm), distance (cm), water total (persisted pulse counter), water (24h total), uptime.

**HTTP endpoints** (all `GET`):

| Endpoint | Returns |
| --- | --- |
| `/` | UI HTML (embedded blob) |
| `/config.json` | UI labels + tank thresholds (embedded blob) |
| `/poll?since=<seq>` | State snapshot + new log lines |
| `/history.json` | Distance + water rings (min/avg/max buckets) |
| `/water/total/set?value=<liters>` | Set the persisted water total counter |
| `/sw?n=<1..8>&on=<0\|1>` | Set one valve |
| `/closeall` | Close all valves |
| `/history/clear?signal=distance\|water&ring=short\|long` | Erase one history ring (RAM + NVS) |
| `/SW?LED=on1\|off1\|...` | Legacy back-compat |

---

## Modbus TCP (port 502)

Same surface as the Web UI, accessible to any HMI / PLC / Home Assistant Modbus integration on the LAN. Full register map: **[doc/MODBUS.md](doc/MODBUS.md)**.

Quick read with [`mbpoll`](https://github.com/epsilonrt/mbpoll):

```bash
# Tank distance × 10 (so 452 = 45.2 cm) and fullness %
mbpoll -m tcp -a 1 -t 3 -r 1 -c 2 <device-ip>

# Open valve 1
mbpoll -m tcp -a 1 -t 0 -r 1 <device-ip> 1

# Close all
mbpoll -m tcp -a 1 -t 0 -r 9 <device-ip> 1
```

`-t 0` = coils, `-t 1` = discrete inputs, `-t 3` = input registers. `mbpoll` uses 1-based addresses; the table in MODBUS.md is in 0-based form so `-r 1` here is address `0`.

---

## OLED display

128 × 64 SSD1306 at I²C `0x3C` shows:

```
192.168.1.200       2d 14:22       <- IP + uptime, small font
45.2 cm  68 %                       <- tank, larger font
Water: 142 L                        <- lifetime pulses since boot, small font
V:1.34..7. I:.2..5..               <- valve + input mask
14:22:31 Valve 1 -> OPEN           <- last log line, HH:MM:SS prefix
```

Layout and font choices live in [src/oled.cpp](src/oled.cpp). The fill-percent thresholds (`EMPTY_CM=140`, `FULL_CM=20`) are hardcoded there in sync with `web/config.json` — the firmware doesn't parse the JSON.

---

## Configuration (`web/config.json`)

Embedded into the firmware at build time and served as-is to the browser. The firmware does **not** parse it — display labels, tank-fullness thresholds, etc. are interpreted by the JS in the browser. Mirror the tank thresholds in `src/oled.cpp` (OLED) and `src/modbus_srv.cpp` (Modbus fullness reg) if you change them.

```json
{
  "title": "Retention Tank",
  "tank": { "empty_distance_cm": 140, "full_distance_cm": 20 },
  "valves": [ { "name": "Valve 1", "note": "" }, ... ],
  "inputs": [ { "name": "Input 1", "note": "Water meter (1 L/pulse)" }, ... ]
}
```

---

## Code conventions

See [CLAUDE.md](CLAUDE.md) for the full guide. Short version:

- C++ formatted with `tools\clang-format.exe -i src\*.cpp src\*.h` before every commit; `.clang-format` at the repo root is the source of truth (Allman braces, 2-space indent, no column wrapping).
- Whitespace-only commits go in `.git-blame-ignore-revs`.
- Doxygen `///` for public APIs in headers; `.cpp` stays clean.
- Default to no inline `//` comments — add one only when the *why* is non-obvious.
- HTTP handlers state their URL and response shape in the `@brief`.

---

## Reference material

- [doc/Hankerila-EA8-board/](doc/Hankerila-EA8-board/) — vendor product PDFs, schematics V1 & V3, demo sketches for every peripheral.
- [doc/datasheets/](doc/datasheets/) — PCF8574, ESP32-WROOM-32.
- [doc/HW modification notes.md](<doc/HW modification notes.md>) — the resistor swaps and INT wiring this firmware assumes.
- [doc/MODBUS.md](doc/MODBUS.md) — register-map reference for HMI integrators.
