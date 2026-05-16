# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

ESP32 firmware for a water retention tank controller running on the **Hankerila HKL-EA8** board. Single Arduino sketch ([src/main.cpp](src/main.cpp)) that exposes an HTTP UI over wired Ethernet (LAN8720 PHY — no Wi-Fi) for driving an 8-channel relay bank wired to **solenoid valves**, reads an HC-SR04 ultrasonic level sensor, and monitors **8 digital inputs** via a second PCF8574 I²C expander.

## Build & flash

PlatformIO Core (>=6.1) is required. On the maintainer's Windows machine `pio` is **not on PATH**; invoke it by full path or use the VS Code PlatformIO extension:

```powershell
# Windows
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run            # build
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t upload  # flash via USB
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" device monitor # serial @ 115200
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t clean
```

CI uses plain `pio run` ([.github/workflows/build.yml](.github/workflows/build.yml)) on push/PR to `main`.

There are no unit tests — the `test/` directory holds only the default PlatformIO placeholder.

## Critical: do not unpin the platform

[platformio.ini](platformio.ini) pins `platform = espressif32@6.10.0`. That version ships `framework-arduinoespressif32 @ 3.20017.x`, i.e. **Arduino-ESP32 core 2.0.17**, which is what [main.cpp](src/main.cpp) targets. Newer espressif32 platforms (7.x+) move to Arduino core 3.x, which:

- removes the legacy `ETH.begin(addr, power, mdc, mdio, type, clk_mode)` signature used in `setup()`
- changes `attachInterrupt` / `IRAM_ATTR` / EEPROM library expectations

Bumping the platform requires a port, not a config change. The header comment at the top of `main.cpp` carries the same warning.

## Architecture / wiring map

All hardware bindings live as `#define`s and `constexpr int`s at the top of [main.cpp](src/main.cpp). Reading those is the fastest way to understand the build, but the non-obvious parts:

- **Ethernet (LAN8720 PHY)**: ETH lib pre-defines `ETH_CLK_MODE` to `ETH_CLOCK_GPIO0_IN`. This board needs `ETH_CLOCK_GPIO17_OUT`, so the sketch does `#undef ETH_CLK_MODE` before redefining ([main.cpp:31](src/main.cpp#L31)). Don't drop the `#undef` — it suppresses a redefinition warning that becomes silent breakage if `ETH.h` is reordered. MDC=GPIO23, MDIO=GPIO18, PHY power not wired (`-1`).
- **Relays**: PCF8574 I²C expander at address `0x24`, SDA=4, SCL=5. **Active-low: write `0` to turn ON, `1` to turn OFF.** All 8 channels are forced OFF in `setup()`.
- **Ultrasonic (HC-SR04)**: trigger=GPIO15, echo=GPIO16. Sampled every 1 s in `loop()`; result cached in `cached_distance_cm` and served via `/poll`. Recorded into a two-ring `History` (10 min / 24 h) persisted to NVS.
- **Digital inputs**: PCF8574 I²C expander at address `0x26`, SDA=4, SCL=5 (same bus as relays). **Active-low: read `0` means input is active.** State packed into `input_mask` (bit i = input i+1 active) and served via `/poll` as `"inputs"`. Hardware modification required — see [doc/HW modification notes.md](doc/HW%20modification%20notes.md):
  - R47 (2 kΩ) and R106 (10 kΩ) removed from the BEEP circuit
  - PCF8574 `/INT` (open-drain, active-low) wired directly to GPIO12
  - GPIO12 configured as `INPUT_PULLUP` in software — **do not add an external pull-up**: GPIO12 is a strapping pin sampled at reset; a hardware pull-up holds it HIGH and switches the flash to 1.8 V mode, corrupting flashing (MD5 mismatch). The software pull-up activates after the strapping phase.
  - ISR (`isr_pcf_input`, `IRAM_ATTR`, `FALLING`) sets `inputs_changed`; `loop()` calls `poll_inputs()` on the flag.
  - `poll_inputs()` uses `Wire.requestFrom(0x26, 1)` directly — **do not replace with `pcf8574_in.digitalRead()` in a loop**. The library's 10 ms latency cache causes all 8 pins to silently return `LOW` when called again within the window, producing a spurious all-inputs-active mask.
- **Network**: `IPAddress local_ip(uint32_t(0))` means DHCP. The fixed-IP line above it is intentionally commented out.

HTTP surface (port 80, single-threaded `WebServer`):
- `GET /` — serve web UI HTML (embedded flash blob)
- `GET /config.json` — serve UI labels/config (embedded flash blob, parsed by browser)
- `GET /poll?since=<seq>` — JSON snapshot: `seq`, `relays`, `distance_cm`, `uptime_ms`, `inputs`, `version`, new log `lines`
- `GET /history.json` — distance history rings (short 24 h / long 30 d) as min/avg/max buckets
- `GET /sw?n=<1..8>&on=<0|1>` — open/close one valve
- `GET /closeall` — close all valves
- `GET /SW?LED=on1..on8|off1..off8` — legacy single-valve toggle (back-compat only)

## Code style

- **Doxygen `///` for public APIs.** Headers carry the contract (`@brief`, `@param`, `@return`, preconditions, lifetime caveats); `.cpp` files stay clean and don't repeat it. See [src/log_buffer.h](src/log_buffer.h) for the pattern.
- **Top-level functions in `main.cpp` get a `///` block.** HTTP handlers state their URL and response shape in the `@brief`.
- **Default to no inline `//` comments.** Add one only when the *why* is non-obvious — library quirk, hardware contract, or workaround.

## Reference material

[doc/Hankerila-EA8-board/](doc/Hankerila-EA8-board/) contains the vendor's product introduction PDFs, schematics (V1 and V3), and **all the original demo sketches** for this exact board (BEEP, RS485, IR send/receive, SHT31, DS18B20, DAC 0–10V, analog 0–5V, MODBUS power meter, ESPHome YAML, etc.). When adding a new peripheral, copy the pin map and init pattern from the matching numbered demo folder rather than rediscovering it — that's the vendor-validated wiring for this hardware revision.
