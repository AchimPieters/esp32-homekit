## [1.3.9]

### Added
- Added missing HAP accessory categories for HomePod (25), Router (33), Audio Receiver (34), TV Set-Top Box (35), and TV Streaming Stick (36).
- Updated README HomeKit category reference table to include the above categories.
- Added Kconfig options for HAP alignment controls: `HOMEKIT_MDNS_PROTOCOL_VERSION`, `HOMEKIT_MDNS_FEATURE_FLAGS`, `HOMEKIT_SETUP_PAYLOAD_FLAGS`, and `HOMEKIT_MDNS_ENABLE_IPV6`.

### Changed
- mDNS feature flags (`ff`) are now configurable through Kconfig instead of being hard-coded.
- Built-in mDNS IPv6 support can now be enabled via Kconfig.

## [1.3.8] - 2026-15-02
- Remove GCC pragma for override-init warnings
- Updated espressif/mdns to 1.9.1
- Make HAP setup payload transport flags configurable via `HOMEKIT_SETUP_PAYLOAD_FLAGS`
- Update advertised HAP mDNS protocol version default to `1.1` via `HOMEKIT_MDNS_PROTOCOL_VERSION`

## [1.2.5] - 2025-06-11

### Added
- Updated wolfssl to 5.8.0 and espressif/mdns to 1.8.2
- Fix base64 decoded size calculation for short inputs

### Removed
- Removed outdated and incorrect information.

## [1.2.4] - 2025-03-13

### Added
- Updated `README.md` file of the LED example.
- Added `homekit_led.png` as an illustration for the LED example.
- Updated the general `README.md` file.

### Removed
- Removed outdated and incorrect information.

## [1.2.0] - 2025-03-12

### Added
- Added `CHANGELOG.md` as Changelog.
- All notable changes to this project will be documented in this file.
- The format is based on [Keep a Changelog](https://keepachangelog.com/), and this project follows [Semantic Versioning](https://semver.org/).
