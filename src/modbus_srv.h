#pragma once

/// @file modbus_srv.h
/// @brief Modbus TCP/IP server (slave) on port 502 exposing the same surface
///        as the Web UI: valves, inputs, tank distance, water meter,
///        history-clear actions.
///
/// Register map is documented in doc/MODBUS.md. The handlers are wired in
/// modbus_srv.cpp via callbacks; on the firmware side it's enough to call
/// modbus_init() once from setup() and modbus_poll() each loop().

/// @brief Bring up the Modbus TCP listener on :502 and wire up the
///        register-map callbacks. Idempotent — call once.
void modbus_init();

/// @brief Reset the flow-rate baseline after the water total is restored or
///        manually changed, so the next flow sample does not include the
///        absolute counter jump.
void modbus_reset_water_flow_baseline();

/// @brief Pump the Modbus library. Cheap; call every loop iteration.
void modbus_poll();
