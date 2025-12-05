# ESP32 HomeKit Codebase Audit and ESP32-C2 Compatibility Review

## Repository Topology and Components
- **Core library (`src/`)**: Implements HomeKit accessory server primitives such as pairing (`pairing.h`/`pairing.c`), storage (`storage.c`/`storage.h`), TLV and JSON helpers (`tlv.c`, `json.c`), MDNS glue (`homekit_mdns*.c`), and platform abstractions (`port.c`).
- **Public headers (`include/`)**: Exposes HomeKit data structures and APIs for accessories and transport integration.
- **Example application (`examples/led/`)**: Minimal HomeKit LED accessory with per-target partition tables and `sdkconfig.defaults` for each supported chip, including ESP32-C2.
- **Packaging metadata (`idf_component.yml`)**: Declares the IDF component, dependency versions, and supported targets.

## Dependency and Build Surface
- Requires ESP-IDF **v5.0 or newer**, with explicit dependencies on `mdns` (>=1.9.1) and `wolfssl` (>=5.8.2~1) as defined in `idf_component.yml`.
- The top-level CMake for the example selects partition tables based on `CONFIG_IDF_TARGET`, with explicit branches for **`esp32c2`**, ensuring the correct memory layout at configure time.
- Default configuration bundles per-target `sdkconfig.defaults`, allowing reproducible builds for each SoC variant without manual menuconfig changes.

## ESP32-C2 Compatibility Findings
- The component manifest lists `esp32c2` among supported targets, so ESP-IDF tooling will permit selection and dependency resolution for ESP32-C2 builds.
- The LED example provides an ESP32-C2-specific partition table (`partitions_esp32c2.csv`) and default configuration (`sdkconfig.defaults.esp32c2`) that set a 26 MHz crystal, 2 MB flash layout, and custom partition offsets, matching common ESP32-C2 module constraints.
- The `idf.py` flow uses `CONFIG_IDF_TARGET` and the per-target defaults, so building for ESP32-C2 only requires `idf.py set-target esp32c2 && idf.py build` from `examples/led`.
- Core source does not contain target-specific conditionals beyond IDF/OpenRTOS portability; it relies on IDF platform APIs for randomness, restart, and mDNS, all provided on ESP32-C2 in IDF v5.x, so no code changes are needed for this target.

## Recommendations
- Use the provided ESP32-C2 defaults when invoking `idf.py` to ensure the correct flash geometry and crystal frequency are applied.
- Keep dependency versions aligned with the manifest (IDF ≥5.0, `mdns` 1.9.x, `wolfssl` 5.8.x) to match the configurations verified in this audit.
- If deploying to ESP32-C2 modules with different flash sizes or crystal frequencies, duplicate and adjust `sdkconfig.defaults.esp32c2` and the corresponding partition CSV to maintain stable OTA/update behavior.
