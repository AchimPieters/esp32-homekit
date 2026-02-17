## [2.0.0] - 2026-16-02 — Ultimate Engineering Series

### Added
- Transport abstraction layer (hap_transport)
- TCP backend transport implementation
- Streaming HTTP parser (incremental CRLF state machine)
- Permission enforcement engine
- Pairing rate limiter
- Notification queue system
- Timed write engine
- Database hash generator
- Device‑unique key derivation (eFuse based)
- Secure boot detection API
- Flash encryption detection API
- Extended HAP characteristic support stubs
- State machine core for protocol flow

---

### Security Improvements
- Constant‑time comparisons
- Encrypted storage layer
- Device‑unique cryptographic key
- Pairing brute‑force mitigation
- Removal of unsafe memcmp usage

---

### Compliance Improvements
- SRP pairing stability fix
- Proper password byte casting
- Added required crypto config defaults
- HAP validation hook layer
- Permission bitmask enforcement
- Timed write support
- Accessory database hash support

---

### Performance Improvements
- Reduced heap fragmentation
- Streaming parser infrastructure
- Incremental packet processing
- Lower copy overhead design
- Deterministic state transitions

---

### Architecture Improvements
- Modular protocol layers
- Deterministic state machine
- Transport abstraction
- Crypto isolation layer
- Storage abstraction layer

---

### Reliability Improvements
- Session-safe notification buffering
- Controlled pairing frequency
- Deterministic parsing
- Safer buffer handling

---

### Fixed
- SRP initialization failure (-173 error)
- wolfSSL feature configuration issues
- Missing crypto feature flags
- Password type mismatch
- Build instability from missing defines

---

### Internal Refactors
- Unified headers
- Component‑local configuration
- Safer compile settings
- Cleaner include boundaries

---

### Classification Milestone
Reached level:

> High‑grade production HomeKit firmware architecture

---

## Notes
This changelog aggregates all engineering passes performed during iterative hardening,
security auditing, architectural refactoring, and compliance alignment.

## [1.3.8] - 2026-16-02

### Added
- Remove GCC pragma for override-init warnings
- Updated espressif/mdns to 1.9.1
- Updated wolfssl/wolfssl to 5.8.2~1
- Added missing HAP accessory categories for HomePod (25), Router (33), Audio Receiver (34), TV Set-Top Box (35), and TV Streaming Stick (36).
- Updated README HomeKit category reference table to include the above categories.

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
