# HomeKit Camera support

This component ships the **HAP protocol layer** for a HomeKit IP Camera
(`include/homekit/camera.h`, `src/camera.c`). It implements everything that is
pure protocol and can run on any ESP32:

- Supported **Video / Audio / RTP Stream Configuration** TLV8 blobs
- **Setup Endpoints** request/response (parses the controller's address + SRTP
  parameters, generates the accessory's SRTP master key/salt and SSRCs)
- **Selected RTP Stream Configuration** commands
  (Start / End / Suspend / Resume / Reconfigure) dispatched to your callbacks
- **Streaming Status** characteristic (with notifications)
- Still-image **snapshots** via the existing `on_resource` callback

## What it does *not* do

It is **not** a media pipeline. Live HomeKit video requires real-time **H.264**
video and AAC-ELD/Opus/PCM audio, RTP packetization and **SRTP** encryption.
The classic ESP32/ESP32-S3 has no H.264 encoder (ESP32-CAM produces MJPEG), so
live streaming is only feasible on hardware with an encoder (e.g. ESP32-P4 or an
external encoder chip). This module hands you the negotiated session — controller
address, ports, SRTP keys/salts and SSRCs — and you feed frames from your H.264
source into your own RTP/SRTP sender. **HomeKit Secure Video (HKSV)** is a
separate, larger effort and is not included.

> Official HAP compliance is gated by Apple's MFi certification program and test
> tools (HAT); shipping code alone cannot grant certification. This module
> targets spec-conformant wire behaviour but has not been certified.

## Declaring a camera accessory

Use accessory category `homekit_accessory_category_ip_camera` and a
`Camera RTP Stream Management` service with the five TLV8 characteristics
(+ optional Streaming Status). Keep references to the characteristics so you can
bind them.

```c
#include <homekit/camera.h>

homekit_characteristic_t cam_supported_video   = HOMEKIT_CHARACTERISTIC_(SUPPORTED_VIDEO_STREAM_CONFIGURATION);
homekit_characteristic_t cam_supported_audio   = HOMEKIT_CHARACTERISTIC_(SUPPORTED_AUDIO_STREAM_CONFIGURATION);
homekit_characteristic_t cam_supported_rtp     = HOMEKIT_CHARACTERISTIC_(SUPPORTED_RTP_CONFIGURATION);
homekit_characteristic_t cam_setup_endpoints   = HOMEKIT_CHARACTERISTIC_(SETUP_ENDPOINTS);
homekit_characteristic_t cam_selected_config   = HOMEKIT_CHARACTERISTIC_(SELECTED_RTP_STREAM_CONFIGURATION);
homekit_characteristic_t cam_streaming_status  = HOMEKIT_CHARACTERISTIC_(STREAMING_STATUS);

// ... place these in a HOMEKIT_SERVICE(CAMERA_RTP_STREAM_MANAGEMENT, ...) inside
// an accessory whose category is homekit_accessory_category_ip_camera.
```

## Wiring the protocol layer

```c
static bool prepare_endpoints(homekit_camera_t *cam, homekit_camera_session_t *s) {
    // Fill the accessory's own RTP endpoint (local IP + the UDP ports your
    // media task bound). Return false to report a Setup Endpoints error.
    s->accessory.ip_version = 0; // IPv4
    strcpy(s->accessory.ip_address, my_local_ip_string);
    s->accessory.video_port = my_video_rtp_port;
    s->accessory.audio_port = my_audio_rtp_port;
    return true;
}

static void on_stream_start(homekit_camera_t *cam, const homekit_camera_session_t *s) {
    // s->controller.{ip_address,video_port,audio_port}        -> where to send media
    // s->controller_video_srtp / s->accessory_video_srtp      -> SRTP keys/salts
    // s->video_ssrc, s->selected_video.{width,height,fps}     -> stream params
    // s->selected_video_max_bitrate (kbps)
    media_task_start(s);
}
static void on_stream_end(homekit_camera_t *cam, const uint8_t id[16]) { media_task_stop(); }

void app_main_setup_camera(void) {
    static homekit_camera_config_t cfg = {
        .h264_params = {{ homekit_camera_h264_profile_main, homekit_camera_h264_level_4_0 }},
        .h264_param_count = 1,
        .attributes = {{1920,1080,30}, {1280,720,30}, {640,480,30}},
        .attribute_count = 3,
        .audio = {{ homekit_camera_audio_codec_opus, 1, homekit_camera_audio_sample_rate_16khz, 20, true }},
        .audio_count = 1,
        .srtp_suites = { homekit_camera_srtp_crypto_aes_cm_128_hmac_sha1_80 },
        .srtp_suite_count = 1,
    };
    static homekit_camera_callbacks_t cb = {
        .prepare_endpoints     = prepare_endpoints,
        .on_stream_start       = on_stream_start,
        .on_stream_end         = on_stream_end,
        // .on_stream_reconfigure / .on_stream_suspend / .on_stream_resume optional
    };

    homekit_camera_t *cam = homekit_camera_new(&cfg, &cb, NULL);
    homekit_camera_bind(cam,
        &cam_supported_video, &cam_supported_audio, &cam_supported_rtp,
        &cam_setup_endpoints, &cam_selected_config, &cam_streaming_status);
}
```

## Snapshots

HomeKit fetches a still image with `POST /resource`
(`{"resource-type":"image","image-width":W,"image-height":H}`). Implement
`config.on_resource` and reply with an HTTP response carrying your JPEG via
`homekit_client_send()`:

```c
void on_resource(const char *body, size_t body_size) {
    // produce a JPEG of the requested size, then:
    // 1) send an "HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\nContent-Length: N\r\n\r\n" header
    // 2) homekit_client_send(jpeg_bytes, jpeg_len);
}
```

## Integrator checklist for live streaming

1. Bind UDP sockets for video/audio RTP, report them in `prepare_endpoints`.
2. On `on_stream_start`, start your H.264 (and audio) encoder at the selected
   resolution/fps/bitrate.
3. Packetize encoded frames into RTP and encrypt with SRTP using the SRTP master
   key/salt from the session, sending to the controller's address/ports.
4. Handle RTCP and the Suspend/Resume/Reconfigure/End commands.
5. On `on_stream_end`, stop the encoder and free sockets.

This module gets you a controller that successfully negotiates a session; steps
1–5 (the media plane) are hardware/codec specific and are your responsibility.

## Live streaming: RTP + SRTP (built-in, tested)

The repository now ships the media-plane primitives needed to send encoded
video to a controller:

- `include/homekit/srtp.h` / `src/srtp.c` — SRTP sender for the mandatory
  `SRTP_AES_CM_128_HMAC_SHA1_80` suite. Self-contained (AES-128 / SHA-1 /
  HMAC-SHA1), validated on the host against **FIPS-197**, **RFC 2202** and the
  **RFC 3711 §B.3** key-derivation test vectors.
- `include/homekit/rtp.h` / `src/rtp.c` — H.264 RTP packetizer (RFC 6184:
  single-NAL and FU-A fragmentation) that SRTP-protects each packet. Validated
  by a protect → decrypt → NAL-reassembly round-trip in CI.

You still provide the **H.264 source** (`esp_h264` — hardware on ESP32-P4,
software on ESP32-S3) and the UDP socket. The wiring from `on_stream_start`:

```c
#include <homekit/rtp.h>
#include <lwip/sockets.h>

static int           g_sock = -1;
static struct sockaddr_in g_dest;
static homekit_rtp_h264_t g_rtp;
static uint8_t       g_rtp_buf[1378];   // SRTP packet scratch (MTU-sized)

static void udp_send(void *user, const uint8_t *pkt, size_t len) {
    (void)user;
    sendto(g_sock, pkt, len, 0, (struct sockaddr *)&g_dest, sizeof(g_dest));
}

static void on_stream_start(homekit_camera_t *cam, const homekit_camera_session_t *s) {
    // Destination = controller's video RTP endpoint.
    g_sock = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&g_dest, 0, sizeof(g_dest));
    g_dest.sin_family = AF_INET;
    g_dest.sin_port   = htons(s->controller.video_port);
    inet_pton(AF_INET, s->controller.ip_address, &g_dest.sin_addr);

    // The accessory sends video to the controller; encrypt it with the SRTP key
    // the controller provided for that stream (controller_video_srtp), and use
    // the accessory video SSRC negotiated during Setup Endpoints.
    homekit_rtp_h264_init(&g_rtp,
        s->controller_video_srtp.master_key,
        s->controller_video_srtp.master_salt,
        s->video_ssrc,
        s->selected_video_rtp_payload_type,
        sizeof(g_rtp_buf), g_rtp_buf,
        udp_send, NULL);

    // ... start your esp_h264 encoder at s->selected_video.{width,height,fps}
    //     and s->selected_video_max_bitrate, then in your media task: ...
}

// Called from your media task for each encoded access unit (Annex-B from
// esp_h264). ts90k is the capture time in 90 kHz units (RTP video clock).
static void on_encoded_frame(const uint8_t *au, size_t au_len, uint32_t ts90k) {
    homekit_rtp_h264_send(&g_rtp, au, au_len, ts90k);
}
```

> **SRTP key direction:** HomeKit's Setup Endpoints exchanges SRTP keys for both
> directions. This example encrypts the outgoing video with
> `controller_video_srtp` (the key the controller supplied for the stream it
> receives). The accessory-generated `accessory_*_srtp` keys are for streams the
> accessory *receives* (e.g. two-way audio). Confirm the exact mapping against
> your controller during bring-up — the crypto and packetization themselves are
> covered by the host test vectors.

### What is and isn't verified
Verified in CI (host): the SRTP crypto (against published vectors) and the
RTP/FU-A framing (round-trip). **Not** verifiable without hardware + a real
controller: the camera capture/encode loop, the UDP transport, RTCP feedback,
and the precise key-direction mapping above. Those are the on-device bring-up
steps.
