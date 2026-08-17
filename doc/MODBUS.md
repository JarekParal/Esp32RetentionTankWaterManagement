# Modbus TCP register map

The firmware exposes a **Modbus TCP server** on `<device-ip>:502` mirroring
the entire Web UI surface. This document is the integrator-facing
reference — for the rest of the project see the [README](../README.md).

## Connection

| | |
| --- | --- |
| Transport | Modbus TCP/IP (MBAP framing, no CRC) |
| Port | `502` |
| Unit ID / slave ID | `1` (libraries default; ignored by this server) |
| Concurrent clients | 1 |
| Byte order | Big-endian (standard Modbus) |
| 32-bit word order | **High word first** (high address = low half) — typical Modbus convention |
| Authentication | None — the server is on the LAN. Put it behind a firewall if exposed. |

## Region overview

| Function code(s) | Region | Range used |
| --- | --- | --- |
| FC1 / FC5 / FC15 (Read / Write / Write multiple coils) | Coils | `0..21` |
| FC2 (Read discrete inputs) | Discrete inputs | `0..7` |
| FC4 (Read input registers) | Input registers | `0..8`, `10..11` |
| FC3 / FC6 / FC16 (Read / Write / Write multiple holding regs) | Holding registers | `0..7` |

Reading addresses outside these ranges returns an illegal-data-address
exception (Modbus error code `2`).

## Coils — `0..21`

Booleans, read/write. Valve coils mirror live state; trigger coils
self-clear to `0` after the action runs.

| Addr | Direction | Meaning |
| ---: | :---: | --- |
| 0 | RW | Valve 1 open (`1`) / closed (`0`) |
| 1 | RW | Valve 2 |
| 2 | RW | Valve 3 |
| 3 | RW | Valve 4 |
| 4 | RW | Valve 5 |
| 5 | RW | Valve 6 |
| 6 | RW | Valve 7 |
| 7 | RW | Valve 8 |
| 8 | W (write-1 trigger) | Close all 8 valves; coil self-clears to 0 |
| 16 | W (write-1 trigger) | **Clear water 24 h history** (RAM + NVS) |
| 17 | W (write-1 trigger) | **Clear water 30 d history** (RAM + NVS) |
| 18 | W (write-1 trigger) | **Clear distance 24 h history** (RAM + NVS) |
| 19 | W (write-1 trigger) | **Clear distance 30 d history** (RAM + NVS) |
| 20 | W (write-1 trigger) | **Clear current 24 h history** (RAM + NVS) |
| 21 | W (write-1 trigger) | **Clear current 30 d history** (RAM + NVS) |

Writing `0` to a trigger coil is a no-op (no action runs, value stays `0`).
The clear actions also log a `Modbus: cleared <signal> <window>` line so the
operation is auditable in the Web UI's Device Log.

## Discrete inputs — `0..7`

Read-only booleans. `1` = input is active. Update latency is bounded by
the PCF8574 `/INT` line wired to GPIO12; transitions are observed within
~1 ms of the edge.

| Addr | Meaning |
| ---: | --- |
| 0 | Input 1 active — **doubles as the water meter (1 L / rising edge)** |
| 1 | Input 2 active |
| 2 | Input 3 active |
| 3 | Input 4 active |
| 4 | Input 5 active |
| 5 | Input 6 active |
| 6 | Input 7 active |
| 7 | Input 8 active |

## Input registers — `0..11`

Read-only 16-bit values. Live measurements and counters. Where a 32-bit
value is split across two registers, the **high word comes first** (lower
address). To reassemble in client code:

```python
hi, lo = read_input_registers(addr_hi, count=2)
value = (hi << 16) | lo
```

| Addr | Meaning | Scale | Range |
| ---: | --- | --- | --- |
| 0 | Distance cm × 10 (HC-SR04) | `cm = reg / 10.0` | 0..6553.5 cm; `0` = no echo |
| 1 | Tank fullness % | integer | 0..100 |
| 2 | Water pulse counter — **high word** | — | 32-bit |
| 3 | Water pulse counter — **low word** | — | 32-bit; persisted to NVS and restored at boot |
| 4 | Water flow rate × 10 (L/min) | `lpm = reg / 10.0` | 0..6553.5 L/min; sampled every 60 s |
| 5 | Water 24 h total L — **high word** | — | 32-bit; sum over the 144 × 10-min buckets |
| 6 | Water 24 h total L — **low word** | — | 32-bit |
| 7 | RMS electrical current | `A = reg / 1000.0` | 0..65.535 A; sampled every 10 s |
| 8 | Estimated active power | `W = reg` | 0..65535 W; assumes 230 V and PF 0.85 |
| 10 | Uptime seconds — **high word** | — | 32-bit, wraps after ~136 years |
| 11 | Uptime seconds — **low word** | — | 32-bit |

Addresses `9` and `12+` are reserved and currently return illegal-data-address.

## Holding registers — `0..7`

Each solenoid has one dedicated read/write holding register. Write `0` to
close it or any non-zero value to open it; reads are normalized to `0` or `1`.
The valve coils at the same addresses remain available for backward compatibility.

| Addr | Meaning |
| ---: | --- |
| 0 | Valve 1 open (`1`) / closed (`0`) |
| 1 | Valve 2 |
| 2 | Valve 3 |
| 3 | Valve 4 |
| 4 | Valve 5 |
| 5 | Valve 6 |
| 6 | Valve 7 |
| 7 | Valve 8 |

## Example sessions

### `mbpoll` CLI

`mbpoll` is the standard Linux command-line tool. **Note `mbpoll` uses
1-based addressing**: pass `-r 1` to read Modbus address `0`.

```bash
# Read tank distance (decimal cm × 10) and fullness % in one go.
mbpoll -m tcp -a 1 -t 3 -r 1 -c 2 192.168.1.200
# -- Polling slave... Ctrl-C to stop)
# [1]: 452       <- 45.2 cm
# [2]: 68        <- 68 %

# Open valve 3 through its dedicated holding register.
mbpoll -m tcp -a 1 -t 4 -r 3 192.168.1.200 1

# Close all 8 valves at once via the trigger coil.
mbpoll -m tcp -a 1 -t 0 -r 9 192.168.1.200 1

# Read 8 inputs.
mbpoll -m tcp -a 1 -t 1 -r 1 -c 8 192.168.1.200
```

### Python (`pymodbus`)

```python
from pymodbus.client import ModbusTcpClient

client = ModbusTcpClient("192.168.1.200", port=502)
client.connect()

# Live readings.
r = client.read_input_registers(address=0, count=12, slave=1)
distance_cm   = r.registers[0] / 10.0
fullness_pct  = r.registers[1]
water_pulses  = (r.registers[2] << 16) | r.registers[3]
flow_lpm      = r.registers[4] / 10.0
water_24h_l   = (r.registers[5] << 16) | r.registers[6]
current_a     = r.registers[7] / 1000.0
power_w       = r.registers[8]
uptime_s      = (r.registers[10] << 16) | r.registers[11]

# Open valve 1 through its dedicated holding register.
client.write_register(address=0, value=1, slave=1)

# Wipe yesterday's water history.
client.write_coil(address=16, value=True, slave=1)

client.close()
```

### Home Assistant `configuration.yaml`

```yaml
modbus:
  - name: retention_tank
    type: tcp
    host: 192.168.1.200
    port: 502

    sensors:
      - name: Tank distance
        unit_of_measurement: cm
        input_type: input
        address: 0
        scale: 0.1
        precision: 1
      - name: Tank fullness
        unit_of_measurement: '%'
        input_type: input
        address: 1
      - name: Water flow
        unit_of_measurement: L/min
        input_type: input
        address: 4
        scale: 0.1
        precision: 1
      - name: Water 24h
        unit_of_measurement: L
        input_type: input
        address: 5
        count: 2
        data_type: uint32
        swap: false
        precision: 0
      - name: RMS current
        unit_of_measurement: A
        input_type: input
        address: 7
        scale: 0.001
        precision: 3
      - name: Estimated active power
        unit_of_measurement: W
        input_type: input
        address: 8
        precision: 0

    switches:
      - name: Valve 1
        address: 0
        write_type: holding
        verify: { input_type: holding, address: 0 }
      # ...valves 2..8 analogously
```

## Update latency

| Register | Updated every | Notes |
| --- | --- | --- |
| Coils 0..7 (valves) | Real-time | Reflects the running `relay_mask` directly |
| Hregs 0..7 (valves) | Real-time | Dedicated read/write register for each solenoid |
| Discrete 0..7 (inputs) | Within ~1 ms of an edge | PCF8574 `/INT` → GPIO12 ISR |
| Ireg 0..1 (distance, fullness) | 10 s | HC-SR04 sample period |
| Ireg 2..3 (water pulses) | Real-time | Counted in `poll_inputs()` on every input change |
| Ireg 4 (flow rate) | 60 s | Independent 1-min sampler in `modbus_srv.cpp` |
| Ireg 5..6 (water 24 h) | Per Modbus read | Computed on-demand from the short-ring buckets |
| Ireg 7 (RMS current) | 10 s | 250 ms differential sampling window on ADS1115 AIN0–AIN1 |
| Ireg 8 (estimated power) | 10 s | `230 V × RMS current × 0.85 power factor` |
| Ireg 10..11 (uptime) | Per Modbus read | `millis() / 1000` |

The History rings themselves roll over on `period_sec` boundaries:
short ring at every 10 minutes UTC-aligned, long ring at midnight UTC.
Water history records each one-liter pulse and graphs bucket sums: 10-minute
totals for the 24-hour ring and daily totals for the 30-day ring. The
independent flow-rate input register remains a 60-second L/min sample.

## Implementation notes

- The Modbus server runs in the main Arduino loop, single-threaded with
  the HTTP server and OTA handler. `mb.task()` is non-blocking; expected
  worst-case latency under load is a handful of milliseconds.
- Trigger coils (`8`, `16..21`) self-clear by returning `0` from their
  `onSetCoil` callback. Reads of these addresses always return `0`.
- Source: [src/modbus_srv.cpp](../src/modbus_srv.cpp), wired into
  `setup()` / `loop()` in [src/main.cpp](../src/main.cpp).
- Library: `emelianov/modbus-esp8266@^4.1.0` — works on Arduino-ESP32 3.x
  despite the `_ESP8266` suffix; types resolve to `NetworkServer` /
  `NetworkClient` from the Arduino core's Network library.
