# Dependency Audit — `espressif/esp_video` 2.2.0

Date: 2026-06-07. Purpose: evaluate `esp_video` for use as the camera-capture /
H.264-encode backend behind the HomeKit camera protocol layer (`src/camera.c`,
`docs/CAMERA.md`), and decide how to add it to this repository.

## 1. Identity
- **Component:** `espressif/esp_video` 2.2.0 (ESP Component Registry).
- **Repo:** https://github.com/espressif/esp-video-components (`esp_video/`).
- **Purpose:** a framework exposing Linux **V4L2** (POSIX `open/ioctl/...`) over
  ESP camera interfaces, multi-camera capture, and V4L2 **M2M codec** devices.
- **Archive size:** ~1.41 MB.

## 2. License
- `esp_video`: **ESPRESSIF MIT** (permissive; compatible with this repo's MIT).
- `esp_cam_sensor`, `esp_sccb_intf`: **Apache-2.0**.
- `esp_h264` (transitive, P4 only): wraps **OpenH264** (Cisco, BSD-2-Clause) and
  TinyH264.
- ⚠️ **H.264 patents:** H.264/AVC is patent-encumbered (MPEG-LA / Via LA pool).
  OpenH264's royalty coverage applies to **Cisco's precompiled binaries**;
  building the software encoder from source for a **commercial** product may
  carry patent-licensing obligations. The ESP32-P4 hardware encoder has its own
  considerations. Not a blocker for hobby/eval use; flagged for productization.

## 3. Targets
Declared targets: `esp32p4, esp32s3, esp32c3, esp32c5, esp32c6, esp32c61, esp32s31`.
**Classic ESP32 is NOT supported.**

| SoC | Capture | HW JPEG | **HW H.264** | Notes |
|-----|---------|---------|--------------|-------|
| **ESP32-P4** | MIPI-CSI, DVP, SPI, USB | ✅ | ✅ `/dev/video11` | full ISP + codec |
| **ESP32-S3** | DVP, SPI, USB | ✅ | ❌ | **capture only** |
| ESP32-C3/C5/C6/C61 | SPI | — | ❌ | SPI capture only |

## 4. ⚠️ Key finding: esp_video does NOT give H.264 on the ESP32-S3
The H.264 M2M encoder device (`/dev/video11`) and the `esp_h264` dependency are
gated to **ESP32-P4 only** (manifest rule `if: target in [esp32p4]`). On the
**ESP32-S3-CAM** boards you mentioned, `esp_video` provides **camera capture +
hardware JPEG, but no H.264**.

To get H.264 on the ESP32-S3 you must add **`espressif/esp_h264`** explicitly —
its **software (OpenH264) encoder** runs on the S3 (~11–17 fps at low
resolution). So a working S3 camera needs **both** `esp_video` (capture) **and**
`esp_h264` (encode). On the P4, `esp_video` already pulls `esp_h264` and uses the
hardware encoder.

## 5. Dependencies (transitive) and toolchain
From the 2.2.0 manifest:
```yaml
dependencies:
  idf: ">=5.4"
  cmake_utilities: "0.*"
  esp_h264:        { version: "1.3.0",  rules: [ if: "target in [esp32p4]" ] }
  usb_host_uvc:    { version: "2.5.*",  rules: [ if: "target in [esp32p4, esp32s3, esp32s31]" ] }
  esp_cam_sensor:  { version: "2.2.*" }
  esp_ipa:         { version: "2.1.*",  rules: [ if: "target in [esp32p4] ] }
```
- **Minimum ESP-IDF: `>=5.4`.** This repo currently declares `idf: >=5.0,<6.0`;
  the camera path therefore needs IDF **5.4+** (a stricter subset).
- Pulls in `esp_cam_sensor` (sensor drivers, incl. **OV2640 via DVP**, **OV5640
  via MIPI/DVP**), `usb_host_uvc` (USB UVC), and on P4 also `esp_ipa` (ISP).

## 6. Quality / security
- Official, actively maintained Espressif component; permissive licensing.
- Large surface (V4L2 layer, ISP, USB-host UVC, sensor drivers). Pulling it into
  a firmware adds significant flash/RAM footprint and several sub-dependencies.
- No CVEs known to this audit; standard Espressif support channels (issues).

## 7. Fit with this repository
The HomeKit **library itself does not call `esp_video`** — `src/camera.c` exposes
callbacks (`prepare_endpoints`, `on_stream_start`, …) and the **integrator**
wires the media plane. So `esp_video` is a dependency of a **camera application /
example**, not of the core HomeKit component.

### Recommendation
- **Do NOT add `esp_video` to the root `idf_component.yml`.** That would force a
  ~1.4 MB camera/V4L2/ISP/USB stack — and raise the minimum IDF to 5.4 — onto
  *every* consumer of this library, including simple light/sensor accessories
  that never touch a camera.
- **Add it scoped to a camera example** (`examples/camera/`). This puts the
  dependency in the repo, available to anyone building a camera, without
  penalising non-camera users. Add **`esp_h264`** alongside it so the S3 has an
  encoder.

This audit's accompanying change adds `examples/camera/main/idf_component.yml`
declaring `esp_video` + `esp_h264` (target-aware), not the root manifest. If you
prefer it in the root manifest regardless of the trade-offs, that is a one-line
follow-up.

## 8. Remaining media-plane work (unchanged by adding this dependency)
`esp_video`/`esp_h264` produce H.264 NALs; HomeKit still needs RTP packetization
+ SRTP encryption using the session keys from the camera protocol layer. That
glue is the next implementation step.

## Sources
- esp_video 2.2.0 — https://components.espressif.com/components/espressif/esp_video/versions/2.2.0/
- esp-video-components (repo) — https://github.com/espressif/esp-video-components
- esp_video README — https://github.com/espressif/esp-video-components/blob/master/esp_video/README.md
- esp_h264 — https://components.espressif.com/components/espressif/esp_h264
- ESP H.264 usage guide — https://developer.espressif.com/blog/2025/07/esp-h264-use-tips/
