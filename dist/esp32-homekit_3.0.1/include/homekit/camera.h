#ifndef __HOMEKIT_CAMERA_H__
#define __HOMEKIT_CAMERA_H__

/**
 * HomeKit IP Camera — HAP protocol layer.
 *
 * This module implements the *protocol* side of a HomeKit camera:
 *   - the Supported Video / Audio / RTP Stream Configuration TLV8 blobs,
 *   - the Setup Endpoints request/response (incl. accessory SRTP key + SSRC),
 *   - the Selected RTP Stream Configuration command parsing
 *     (start / stop / suspend / resume / reconfigure),
 *   - the Streaming Status characteristic.
 *
 * It does NOT contain a media pipeline. Real-time H.264 video and audio
 * encoding, RTP packetization and SRTP encryption are the integrator's
 * responsibility: the module hands you the negotiated session (controller +
 * accessory address, ports, SRTP keys/salts and SSRCs) via callbacks, and you
 * feed frames from your H.264 source (e.g. an external encoder or an
 * ESP32-P4-class device) into your own RTP/SRTP sender.
 *
 * Still-image snapshots are handled separately through the existing
 * `on_resource` callback in homekit_server_config_t.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <homekit/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---- Codec / parameter enumerations (HAP R2) ----

typedef enum {
        homekit_camera_video_codec_h264 = 0,
} homekit_camera_video_codec_t;

typedef enum {
        homekit_camera_h264_profile_constrained_baseline = 0,
        homekit_camera_h264_profile_main = 1,
        homekit_camera_h264_profile_high = 2,
} homekit_camera_h264_profile_t;

typedef enum {
        homekit_camera_h264_level_3_1 = 0,
        homekit_camera_h264_level_3_2 = 1,
        homekit_camera_h264_level_4_0 = 2,
} homekit_camera_h264_level_t;

typedef enum {
        homekit_camera_audio_codec_pcmu = 0,
        homekit_camera_audio_codec_pcma = 1,
        homekit_camera_audio_codec_aac_eld = 2,
        homekit_camera_audio_codec_opus = 3,
        homekit_camera_audio_codec_msbc = 4,
        homekit_camera_audio_codec_amr = 5,
        homekit_camera_audio_codec_amr_wb = 6,
} homekit_camera_audio_codec_t;

typedef enum {
        homekit_camera_audio_sample_rate_8khz = 0,
        homekit_camera_audio_sample_rate_16khz = 1,
        homekit_camera_audio_sample_rate_24khz = 2,
} homekit_camera_audio_sample_rate_t;

typedef enum {
        homekit_camera_srtp_crypto_aes_cm_128_hmac_sha1_80 = 0,
        homekit_camera_srtp_crypto_aes_256_cm_hmac_sha1_80 = 1,
        homekit_camera_srtp_crypto_none = 2,
} homekit_camera_srtp_crypto_suite_t;

typedef enum {
        homekit_camera_streaming_status_available = 0,
        homekit_camera_streaming_status_in_use = 1,
        homekit_camera_streaming_status_unavailable = 2,
} homekit_camera_streaming_status_t;

// ---- Accessory capability description ----

typedef struct {
        uint16_t width;
        uint16_t height;
        uint8_t  fps;
} homekit_camera_video_attributes_t;

typedef struct {
        homekit_camera_h264_profile_t profile;
        homekit_camera_h264_level_t   level;
} homekit_camera_h264_param_t;

typedef struct {
        homekit_camera_audio_codec_t       codec;
        uint8_t                            channels;       // typically 1
        homekit_camera_audio_sample_rate_t sample_rate;
        uint8_t                            rtp_time;       // packet time in ms (e.g. 20/30/60)
        bool                               variable_bitrate;
} homekit_camera_audio_config_t;

// Up to these many entries are advertised; tune to taste / RAM.
#define HOMEKIT_CAMERA_MAX_ATTRIBUTES   16
#define HOMEKIT_CAMERA_MAX_H264_PARAMS  8
#define HOMEKIT_CAMERA_MAX_AUDIO_CONFIGS 4
#define HOMEKIT_CAMERA_MAX_SRTP_SUITES  3

typedef struct {
        // Supported video.
        homekit_camera_h264_param_t       h264_params[HOMEKIT_CAMERA_MAX_H264_PARAMS];
        size_t                            h264_param_count;
        homekit_camera_video_attributes_t attributes[HOMEKIT_CAMERA_MAX_ATTRIBUTES];
        size_t                            attribute_count;

        // Supported audio.
        homekit_camera_audio_config_t     audio[HOMEKIT_CAMERA_MAX_AUDIO_CONFIGS];
        size_t                            audio_count;
        bool                              comfort_noise;

        // Supported SRTP crypto suites.
        homekit_camera_srtp_crypto_suite_t srtp_suites[HOMEKIT_CAMERA_MAX_SRTP_SUITES];
        size_t                             srtp_suite_count;
} homekit_camera_config_t;

// ---- Negotiated session ----

typedef struct {
        uint8_t  crypto_suite;     // homekit_camera_srtp_crypto_suite_t
        uint8_t  master_key[32];
        size_t   master_key_size;  // 16 (AES-128) or 32 (AES-256)
        uint8_t  master_salt[14];
        size_t   master_salt_size; // 14
} homekit_camera_srtp_params_t;

typedef struct {
        uint8_t  ip_version;       // 0 = IPv4, 1 = IPv6
        char     ip_address[46];   // textual address (max IPv6 length)
        uint16_t video_port;
        uint16_t audio_port;
} homekit_camera_address_t;

typedef enum {
        homekit_camera_command_end = 0,
        homekit_camera_command_start = 1,
        homekit_camera_command_suspend = 2,
        homekit_camera_command_resume = 3,
        homekit_camera_command_reconfigure = 4,
} homekit_camera_session_command_t;

typedef struct {
        uint8_t  session_id[16];

        // Controller endpoint (where the accessory must send media to).
        homekit_camera_address_t     controller;
        homekit_camera_srtp_params_t controller_video_srtp;
        homekit_camera_srtp_params_t controller_audio_srtp;

        // Accessory endpoint (filled in by the module during Setup Endpoints).
        homekit_camera_address_t     accessory;
        homekit_camera_srtp_params_t accessory_video_srtp;
        homekit_camera_srtp_params_t accessory_audio_srtp;
        uint32_t                     video_ssrc;
        uint32_t                     audio_ssrc;

        // Selected stream parameters (filled in on Start/Reconfigure).
        homekit_camera_video_attributes_t selected_video;
        uint32_t                          selected_video_max_bitrate; // kbps
        uint8_t                           selected_video_rtp_payload_type;
        bool                              has_selected_video;
} homekit_camera_session_t;

// ---- Integrator callbacks ----
//
// All callbacks are invoked from the HomeKit server task. Keep them short:
// hand work off to your media task rather than blocking here.

typedef struct homekit_camera homekit_camera_t;

typedef struct {
        // Invoked during Setup Endpoints, before the response is built. The
        // integrator must fill session->accessory (ip_version, ip_address,
        // video_port, audio_port) with the accessory's own RTP endpoint (the
        // local IP and the UDP ports its media stack bound). Return true on
        // success; return false to report a Setup Endpoints error to the
        // controller. If NULL, the module mirrors the controller's IP version
        // and leaves ports at 0 (protocol completes but no media will flow).
        bool (*prepare_endpoints)(homekit_camera_t *camera, homekit_camera_session_t *session);

        // A controller selected a stream and issued Start. `session` is owned by
        // the module and remains valid until on_stream_end for the same id.
        void (*on_stream_start)(homekit_camera_t *camera, const homekit_camera_session_t *session);
        // Controller issued Reconfigure (bitrate/attributes changed).
        void (*on_stream_reconfigure)(homekit_camera_t *camera, const homekit_camera_session_t *session);
        // Controller issued Suspend / Resume.
        void (*on_stream_suspend)(homekit_camera_t *camera, const uint8_t session_id[16]);
        void (*on_stream_resume)(homekit_camera_t *camera, const uint8_t session_id[16]);
        // Controller issued End (or session torn down). Stop sending media.
        void (*on_stream_end)(homekit_camera_t *camera, const uint8_t session_id[16]);
} homekit_camera_callbacks_t;

struct homekit_camera {
        homekit_camera_config_t    config;
        homekit_camera_callbacks_t callbacks;
        void                      *context; // integrator-owned

        // Internal state (managed by the module).
        homekit_camera_session_t   session;       // single active session
        bool                       session_active;
        bool                       have_setup;     // a Setup Endpoints write happened
        uint8_t                    last_setup_status; // 0=Success,1=Busy,2=Error
        homekit_camera_streaming_status_t status;

        homekit_characteristic_t  *ch_streaming_status;
};

// ---- API ----

// Allocate and initialize a camera context. Returns NULL on allocation failure.
homekit_camera_t *homekit_camera_new(
        const homekit_camera_config_t *config,
        const homekit_camera_callbacks_t *callbacks,
        void *context
        );

void homekit_camera_free(homekit_camera_t *camera);

// Wire the module's getters/setters onto the four TLV8 characteristics of a
// Camera RTP Stream Management service. Pass the characteristics you declared
// with the HOMEKIT_DECLARE_CHARACTERISTIC_* macros. `streaming_status` is
// optional (may be NULL) but recommended.
//
// Returns 0 on success.
int homekit_camera_bind(
        homekit_camera_t *camera,
        homekit_characteristic_t *supported_video,
        homekit_characteristic_t *supported_audio,
        homekit_characteristic_t *supported_rtp,
        homekit_characteristic_t *setup_endpoints,
        homekit_characteristic_t *selected_config,
        homekit_characteristic_t *streaming_status
        );

// Update and notify the Streaming Status characteristic.
void homekit_camera_set_status(homekit_camera_t *camera, homekit_camera_streaming_status_t status);

#ifdef __cplusplus
}
#endif

#endif // __HOMEKIT_CAMERA_H__
