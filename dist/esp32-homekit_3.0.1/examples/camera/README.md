# HomeKit camera example (scaffold)

This example wires Espressif's camera stack to the HomeKit camera protocol layer
(`include/homekit/camera.h`, `src/camera.c`).

> **Status: dependency + integration scaffold.** It declares the required
> components and documents the data flow. The media-plane glue (RTP packetization
> + SRTP) is the remaining implementation step — see `docs/CAMERA.md`.

## Dependencies (see `main/idf_component.yml`)
- **`espressif/esp_video`** (`~2.2.0`) — V4L2 camera capture; hardware H.264 on
  ESP32-P4. Requires **ESP-IDF ≥ 5.4**.
- **`espressif/esp_h264`** (`~1.3.0`) — H.264 encoder: **hardware on ESP32-P4**,
  **software (OpenH264) on ESP32-S3**.
- `esp32-homekit` (this repo, local path).

A full dependency audit is in [`docs/AUDIT_esp_video_2.2.0.md`](../../docs/AUDIT_esp_video_2.2.0.md).

## Hardware support
| Board / SoC | Capture | H.264 | Practical result |
|-------------|---------|-------|------------------|
| ESP32-P4 + MIPI camera (e.g. OV5640) | esp_video | **hardware** | ~1080p30 live stream |
| ESP32-S3-CAM (OV2640 DVP) | esp_video | **software** (esp_h264) | low-res, ~10–17 fps |
| Classic ESP32 | not supported by esp_video | — | snapshots only |

> The OV2640/OV5640 sensors output JPEG/YUV, not H.264. HomeKit **snapshots**
> (still images) work today with no encoder via the `on_resource` hook.

## Data flow for live streaming
```
camera sensor ──(V4L2 capture, esp_video)──▶ YUV420 frame
      │
      ▼
esp_h264 encoder (HW on P4 / SW on S3) ──▶ H.264 NAL units
      │
      ▼
RTP packetization + SRTP encryption  ◀── session keys/SSRC/ports from
      │                                   homekit_camera_t on_stream_start()
      ▼
UDP send to the controller's RTP endpoint
```

The HomeKit handshake (Pair-Verify, Setup Endpoints, Selected RTP Stream
Configuration) is handled by `src/camera.c`; on `on_stream_start` you receive the
negotiated `homekit_camera_session_t` (controller address/ports, SRTP master
keys/salts, SSRCs, selected resolution/bitrate) to drive the encoder + sender.
