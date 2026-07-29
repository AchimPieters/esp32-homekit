/**

   Copyright 2026 Achim Pieters | StudioPieters®

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NON INFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   for more information visit https://www.studiopieters.nl

 **/

#include <stdlib.h>
#include <string.h>

#include <homekit/camera.h>
#include <homekit/characteristics.h>
#include <homekit/tlv.h>

#include "port.h"
#include "debug.h"

// ---------------------------------------------------------------------------
// HAP TLV type constants (HAP-R2, Camera RTP Stream Management)
// ---------------------------------------------------------------------------

// Supported Video Stream Configuration
enum {
        VIDEO_CONFIG_CODEC = 1,
};
enum {
        VIDEO_CODEC_TYPE = 1,
        VIDEO_CODEC_PARAMS = 2,
        VIDEO_CODEC_ATTRIBUTES = 3,
};
enum {
        VIDEO_CODEC_PARAM_PROFILE = 1,
        VIDEO_CODEC_PARAM_LEVEL = 2,
        VIDEO_CODEC_PARAM_PACKETIZATION = 3,
};
enum {
        VIDEO_ATTR_WIDTH = 1,
        VIDEO_ATTR_HEIGHT = 2,
        VIDEO_ATTR_FPS = 3,
};

// Supported Audio Stream Configuration
enum {
        AUDIO_CONFIG_CODEC = 1,
        AUDIO_CONFIG_COMFORT_NOISE = 2,
};
enum {
        AUDIO_CODEC_TYPE = 1,
        AUDIO_CODEC_PARAMS = 2,
};
enum {
        AUDIO_CODEC_PARAM_CHANNELS = 1,
        AUDIO_CODEC_PARAM_BITRATE = 2,
        AUDIO_CODEC_PARAM_SAMPLERATE = 3,
        AUDIO_CODEC_PARAM_RTP_TIME = 4,
};

// Supported RTP Configuration
enum {
        RTP_CONFIG_SRTP_CRYPTO_SUITE = 2,
};

// Setup Endpoints
enum {
        SETUP_SESSION_ID = 1,
        SETUP_STATUS = 2,            // (response only)
        SETUP_ADDRESS = 3,
        SETUP_SRTP_VIDEO = 4,
        SETUP_SRTP_AUDIO = 5,
        SETUP_SSRC_VIDEO = 6,        // (response only)
        SETUP_SSRC_AUDIO = 7,        // (response only)
};
enum {
        ADDRESS_IP_VERSION = 1,
        ADDRESS_IP = 2,
        ADDRESS_VIDEO_RTP_PORT = 3,
        ADDRESS_AUDIO_RTP_PORT = 4,
};
enum {
        SRTP_CRYPTO_SUITE = 1,
        SRTP_MASTER_KEY = 2,
        SRTP_MASTER_SALT = 3,
};

// Selected RTP Stream Configuration
enum {
        SELECTED_SESSION_CONTROL = 1,
        SELECTED_VIDEO_PARAMS = 2,
        SELECTED_AUDIO_PARAMS = 3,
};
enum {
        SESSION_CONTROL_ID = 1,
        SESSION_CONTROL_COMMAND = 2,
};
enum {
        SELECTED_VIDEO_RTP_PARAMS = 4,
};
enum {
        VIDEO_RTP_PAYLOAD_TYPE = 1,
        VIDEO_RTP_SSRC = 2,
        VIDEO_RTP_MAX_BITRATE = 3,
};

#define SETUP_STATUS_SUCCESS 0
#define SETUP_STATUS_BUSY    1
#define SETUP_STATUS_ERROR   2

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

homekit_camera_t *homekit_camera_new(
        const homekit_camera_config_t *config,
        const homekit_camera_callbacks_t *callbacks,
        void *context
        ) {
        if (!config)
                return NULL;

        homekit_camera_t *camera = calloc(1, sizeof(homekit_camera_t));
        if (!camera)
                return NULL;

        camera->config = *config;
        if (callbacks)
                camera->callbacks = *callbacks;
        camera->context = context;
        camera->status = homekit_camera_streaming_status_available;
        camera->last_setup_status = SETUP_STATUS_SUCCESS;

        return camera;
}

void homekit_camera_free(homekit_camera_t *camera) {
        free(camera);
}

// ---------------------------------------------------------------------------
// Supported configuration builders
// ---------------------------------------------------------------------------

static tlv_values_t *build_supported_video(const homekit_camera_config_t *c) {
        tlv_values_t *root = tlv_new();
        if (!root)
                return NULL;

        tlv_values_t *codec = tlv_new();
        if (!codec) {
                tlv_free(root);
                return NULL;
        }

        tlv_add_integer_value(codec, VIDEO_CODEC_TYPE, 1, homekit_camera_video_codec_h264);

        for (size_t i = 0; i < c->h264_param_count; i++) {
                tlv_values_t *params = tlv_new();
                if (!params) continue;
                tlv_add_integer_value(params, VIDEO_CODEC_PARAM_PROFILE, 1, c->h264_params[i].profile);
                tlv_add_integer_value(params, VIDEO_CODEC_PARAM_LEVEL, 1, c->h264_params[i].level);
                tlv_add_integer_value(params, VIDEO_CODEC_PARAM_PACKETIZATION, 1, 0); // non-interleaved
                tlv_add_tlv_value(codec, VIDEO_CODEC_PARAMS, params);
                tlv_free(params);
        }

        for (size_t i = 0; i < c->attribute_count; i++) {
                tlv_values_t *attr = tlv_new();
                if (!attr) continue;
                tlv_add_integer_value(attr, VIDEO_ATTR_WIDTH, 2, c->attributes[i].width);
                tlv_add_integer_value(attr, VIDEO_ATTR_HEIGHT, 2, c->attributes[i].height);
                tlv_add_integer_value(attr, VIDEO_ATTR_FPS, 1, c->attributes[i].fps);
                tlv_add_tlv_value(codec, VIDEO_CODEC_ATTRIBUTES, attr);
                tlv_free(attr);
        }

        tlv_add_tlv_value(root, VIDEO_CONFIG_CODEC, codec);
        tlv_free(codec);
        return root;
}

static tlv_values_t *build_supported_audio(const homekit_camera_config_t *c) {
        tlv_values_t *root = tlv_new();
        if (!root)
                return NULL;

        for (size_t i = 0; i < c->audio_count; i++) {
                tlv_values_t *codec = tlv_new();
                if (!codec) continue;
                tlv_add_integer_value(codec, AUDIO_CODEC_TYPE, 1, c->audio[i].codec);

                tlv_values_t *params = tlv_new();
                if (params) {
                        tlv_add_integer_value(params, AUDIO_CODEC_PARAM_CHANNELS, 1,
                                              c->audio[i].channels ? c->audio[i].channels : 1);
                        tlv_add_integer_value(params, AUDIO_CODEC_PARAM_BITRATE, 1,
                                              c->audio[i].variable_bitrate ? 0 : 1);
                        tlv_add_integer_value(params, AUDIO_CODEC_PARAM_SAMPLERATE, 1, c->audio[i].sample_rate);
                        if (c->audio[i].rtp_time)
                                tlv_add_integer_value(params, AUDIO_CODEC_PARAM_RTP_TIME, 1, c->audio[i].rtp_time);
                        tlv_add_tlv_value(codec, AUDIO_CODEC_PARAMS, params);
                        tlv_free(params);
                }

                tlv_add_tlv_value(root, AUDIO_CONFIG_CODEC, codec);
                tlv_free(codec);
        }

        tlv_add_integer_value(root, AUDIO_CONFIG_COMFORT_NOISE, 1, c->comfort_noise ? 1 : 0);
        return root;
}

static tlv_values_t *build_supported_rtp(const homekit_camera_config_t *c) {
        tlv_values_t *root = tlv_new();
        if (!root)
                return NULL;

        size_t n = c->srtp_suite_count;
        if (n == 0) {
                // Default: advertise the mandatory AES-128 suite.
                tlv_add_integer_value(root, RTP_CONFIG_SRTP_CRYPTO_SUITE, 1,
                                      homekit_camera_srtp_crypto_aes_cm_128_hmac_sha1_80);
                return root;
        }
        for (size_t i = 0; i < n; i++)
                tlv_add_integer_value(root, RTP_CONFIG_SRTP_CRYPTO_SUITE, 1, c->srtp_suites[i]);
        return root;
}

static tlv_values_t *build_streaming_status(homekit_camera_streaming_status_t status) {
        tlv_values_t *root = tlv_new();
        if (!root)
                return NULL;
        tlv_add_integer_value(root, 1 /* status */, 1, status);
        return root;
}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------

#define CAMERA_OF(ch) ((homekit_camera_t *)(ch)->context)

static homekit_value_t get_supported_video(const homekit_characteristic_t *ch) {
        return HOMEKIT_TLV(build_supported_video(&CAMERA_OF(ch)->config));
}

static homekit_value_t get_supported_audio(const homekit_characteristic_t *ch) {
        return HOMEKIT_TLV(build_supported_audio(&CAMERA_OF(ch)->config));
}

static homekit_value_t get_supported_rtp(const homekit_characteristic_t *ch) {
        return HOMEKIT_TLV(build_supported_rtp(&CAMERA_OF(ch)->config));
}

static homekit_value_t get_streaming_status(const homekit_characteristic_t *ch) {
        return HOMEKIT_TLV(build_streaming_status(CAMERA_OF(ch)->status));
}

// ---------------------------------------------------------------------------
// Setup Endpoints
// ---------------------------------------------------------------------------

static void parse_srtp_params(tlv_values_t *t, homekit_camera_srtp_params_t *out) {
        memset(out, 0, sizeof(*out));
        out->crypto_suite = tlv_get_integer_value(t, SRTP_CRYPTO_SUITE, homekit_camera_srtp_crypto_none);

        tlv_t *key = tlv_get_value(t, SRTP_MASTER_KEY);
        if (key && key->size <= sizeof(out->master_key)) {
                memcpy(out->master_key, key->value, key->size);
                out->master_key_size = key->size;
        }
        tlv_t *salt = tlv_get_value(t, SRTP_MASTER_SALT);
        if (salt && salt->size <= sizeof(out->master_salt)) {
                memcpy(out->master_salt, salt->value, salt->size);
                out->master_salt_size = salt->size;
        }
}

static void parse_address(tlv_values_t *t, homekit_camera_address_t *out) {
        memset(out, 0, sizeof(*out));
        out->ip_version = tlv_get_integer_value(t, ADDRESS_IP_VERSION, 0);
        tlv_t *ip = tlv_get_value(t, ADDRESS_IP);
        if (ip && ip->size < sizeof(out->ip_address)) {
                memcpy(out->ip_address, ip->value, ip->size);
                out->ip_address[ip->size] = 0;
        }
        out->video_port = tlv_get_integer_value(t, ADDRESS_VIDEO_RTP_PORT, 0);
        out->audio_port = tlv_get_integer_value(t, ADDRESS_AUDIO_RTP_PORT, 0);
}

static void generate_srtp(homekit_camera_srtp_params_t *p, uint8_t crypto_suite) {
        p->crypto_suite = crypto_suite;
        p->master_key_size =
                (crypto_suite == homekit_camera_srtp_crypto_aes_256_cm_hmac_sha1_80) ? 32 : 16;
        p->master_salt_size = 14;
        homekit_random_fill(p->master_key, p->master_key_size);
        homekit_random_fill(p->master_salt, p->master_salt_size);
}

static void add_srtp_tlv(tlv_values_t *parent, uint8_t type, const homekit_camera_srtp_params_t *p) {
        tlv_values_t *t = tlv_new();
        if (!t) return;
        tlv_add_integer_value(t, SRTP_CRYPTO_SUITE, 1, p->crypto_suite);
        tlv_add_value(t, SRTP_MASTER_KEY, p->master_key, p->master_key_size);
        tlv_add_value(t, SRTP_MASTER_SALT, p->master_salt, p->master_salt_size);
        tlv_add_tlv_value(parent, type, t);
        tlv_free(t);
}

static void add_address_tlv(tlv_values_t *parent, uint8_t type, const homekit_camera_address_t *a) {
        tlv_values_t *t = tlv_new();
        if (!t) return;
        tlv_add_integer_value(t, ADDRESS_IP_VERSION, 1, a->ip_version);
        tlv_add_string_value(t, ADDRESS_IP, a->ip_address);
        tlv_add_integer_value(t, ADDRESS_VIDEO_RTP_PORT, 2, a->video_port);
        tlv_add_integer_value(t, ADDRESS_AUDIO_RTP_PORT, 2, a->audio_port);
        tlv_add_tlv_value(parent, type, t);
        tlv_free(t);
}

static void setup_endpoints_set(homekit_characteristic_t *ch, const homekit_value_t value) {
        homekit_camera_t *camera = CAMERA_OF(ch);
        if (value.format != homekit_format_tlv || !value.tlv_values) {
                camera->have_setup = true;
                camera->last_setup_status = SETUP_STATUS_ERROR;
                return;
        }
        tlv_values_t *m = value.tlv_values;

        homekit_camera_session_t *s = &camera->session;
        memset(s, 0, sizeof(*s));

        tlv_t *sid = tlv_get_value(m, SETUP_SESSION_ID);
        if (sid && sid->size == sizeof(s->session_id))
                memcpy(s->session_id, sid->value, sizeof(s->session_id));

        tlv_values_t *addr = tlv_get_tlv_value(m, SETUP_ADDRESS);
        if (addr) { parse_address(addr, &s->controller); tlv_free(addr); }

        tlv_values_t *vsrtp = tlv_get_tlv_value(m, SETUP_SRTP_VIDEO);
        if (vsrtp) { parse_srtp_params(vsrtp, &s->controller_video_srtp); tlv_free(vsrtp); }

        tlv_values_t *asrtp = tlv_get_tlv_value(m, SETUP_SRTP_AUDIO);
        if (asrtp) { parse_srtp_params(asrtp, &s->controller_audio_srtp); tlv_free(asrtp); }

        // Accessory side: generate our SRTP material and SSRCs.
        generate_srtp(&s->accessory_video_srtp, s->controller_video_srtp.crypto_suite);
        generate_srtp(&s->accessory_audio_srtp, s->controller_audio_srtp.crypto_suite);
        homekit_random_fill((uint8_t *)&s->video_ssrc, sizeof(s->video_ssrc));
        homekit_random_fill((uint8_t *)&s->audio_ssrc, sizeof(s->audio_ssrc));

        // Let the integrator fill the accessory address (local IP + bound ports).
        uint8_t status = SETUP_STATUS_SUCCESS;
        if (camera->callbacks.prepare_endpoints) {
                if (!camera->callbacks.prepare_endpoints(camera, s))
                        status = SETUP_STATUS_ERROR;
        } else {
                s->accessory.ip_version = s->controller.ip_version;
        }

        camera->have_setup = true;
        camera->last_setup_status = status;
}

static homekit_value_t setup_endpoints_get(const homekit_characteristic_t *ch) {
        homekit_camera_t *camera = CAMERA_OF(ch);

        tlv_values_t *root = tlv_new();
        if (!root)
                return HOMEKIT_TLV(NULL);

        if (!camera->have_setup) {
                return HOMEKIT_TLV(root);
        }

        homekit_camera_session_t *s = &camera->session;
        tlv_add_value(root, SETUP_SESSION_ID, s->session_id, sizeof(s->session_id));
        tlv_add_integer_value(root, SETUP_STATUS, 1, camera->last_setup_status);

        if (camera->last_setup_status == SETUP_STATUS_SUCCESS) {
                add_address_tlv(root, SETUP_ADDRESS, &s->accessory);
                add_srtp_tlv(root, SETUP_SRTP_VIDEO, &s->accessory_video_srtp);
                add_srtp_tlv(root, SETUP_SRTP_AUDIO, &s->accessory_audio_srtp);
                tlv_add_integer_value(root, SETUP_SSRC_VIDEO, 4, (int)s->video_ssrc);
                tlv_add_integer_value(root, SETUP_SSRC_AUDIO, 4, (int)s->audio_ssrc);
        }

        return HOMEKIT_TLV(root);
}

// ---------------------------------------------------------------------------
// Selected RTP Stream Configuration
// ---------------------------------------------------------------------------

static void parse_selected_video(homekit_camera_t *camera, tlv_values_t *vp) {
        homekit_camera_session_t *s = &camera->session;

        tlv_values_t *attr = tlv_get_tlv_value(vp, VIDEO_CODEC_ATTRIBUTES);
        if (attr) {
                s->selected_video.width = tlv_get_integer_value(attr, VIDEO_ATTR_WIDTH, 0);
                s->selected_video.height = tlv_get_integer_value(attr, VIDEO_ATTR_HEIGHT, 0);
                s->selected_video.fps = tlv_get_integer_value(attr, VIDEO_ATTR_FPS, 0);
                tlv_free(attr);
        }

        tlv_values_t *rtp = tlv_get_tlv_value(vp, SELECTED_VIDEO_RTP_PARAMS);
        if (rtp) {
                s->selected_video_rtp_payload_type =
                        tlv_get_integer_value(rtp, VIDEO_RTP_PAYLOAD_TYPE, 99);
                s->selected_video_max_bitrate =
                        tlv_get_integer_value(rtp, VIDEO_RTP_MAX_BITRATE, 0);
                tlv_free(rtp);
        }
        s->has_selected_video = true;
}

static void selected_config_set(homekit_characteristic_t *ch, const homekit_value_t value) {
        homekit_camera_t *camera = CAMERA_OF(ch);
        if (value.format != homekit_format_tlv || !value.tlv_values)
                return;
        tlv_values_t *m = value.tlv_values;

        tlv_values_t *ctrl = tlv_get_tlv_value(m, SELECTED_SESSION_CONTROL);
        if (!ctrl)
                return;

        uint8_t command = tlv_get_integer_value(ctrl, SESSION_CONTROL_COMMAND,
                                                 homekit_camera_command_end);
        tlv_t *sid = tlv_get_value(ctrl, SESSION_CONTROL_ID);
        uint8_t session_id[16] = {0};
        if (sid && sid->size == sizeof(session_id))
                memcpy(session_id, sid->value, sizeof(session_id));
        tlv_free(ctrl);

        switch (command) {
        case homekit_camera_command_start:
        case homekit_camera_command_reconfigure: {
                tlv_values_t *vp = tlv_get_tlv_value(m, SELECTED_VIDEO_PARAMS);
                if (vp) { parse_selected_video(camera, vp); tlv_free(vp); }

                if (command == homekit_camera_command_start) {
                        camera->session_active = true;
                        homekit_camera_set_status(camera, homekit_camera_streaming_status_in_use);
                        if (camera->callbacks.on_stream_start)
                                camera->callbacks.on_stream_start(camera, &camera->session);
                } else {
                        if (camera->callbacks.on_stream_reconfigure)
                                camera->callbacks.on_stream_reconfigure(camera, &camera->session);
                }
                break;
        }
        case homekit_camera_command_suspend:
                if (camera->callbacks.on_stream_suspend)
                        camera->callbacks.on_stream_suspend(camera, session_id);
                break;
        case homekit_camera_command_resume:
                if (camera->callbacks.on_stream_resume)
                        camera->callbacks.on_stream_resume(camera, session_id);
                break;
        case homekit_camera_command_end:
        default:
                camera->session_active = false;
                if (camera->callbacks.on_stream_end)
                        camera->callbacks.on_stream_end(camera, session_id);
                homekit_camera_set_status(camera, homekit_camera_streaming_status_available);
                break;
        }
}

static homekit_value_t selected_config_get(const homekit_characteristic_t *ch) {
        (void)ch;
        // The Selected RTP Stream Configuration read returns the last selection;
        // an empty TLV is acceptable when nothing is selected.
        return HOMEKIT_TLV(tlv_new());
}

// ---------------------------------------------------------------------------
// Binding
// ---------------------------------------------------------------------------

int homekit_camera_bind(
        homekit_camera_t *camera,
        homekit_characteristic_t *supported_video,
        homekit_characteristic_t *supported_audio,
        homekit_characteristic_t *supported_rtp,
        homekit_characteristic_t *setup_endpoints,
        homekit_characteristic_t *selected_config,
        homekit_characteristic_t *streaming_status
        ) {
        if (!camera || !supported_video || !supported_audio || !supported_rtp ||
            !setup_endpoints || !selected_config)
                return -1;

        supported_video->context = camera;
        supported_video->getter_ex = get_supported_video;

        supported_audio->context = camera;
        supported_audio->getter_ex = get_supported_audio;

        supported_rtp->context = camera;
        supported_rtp->getter_ex = get_supported_rtp;

        setup_endpoints->context = camera;
        setup_endpoints->getter_ex = setup_endpoints_get;
        setup_endpoints->setter_ex = setup_endpoints_set;

        selected_config->context = camera;
        selected_config->getter_ex = selected_config_get;
        selected_config->setter_ex = selected_config_set;

        if (streaming_status) {
                streaming_status->context = camera;
                streaming_status->getter_ex = get_streaming_status;
                camera->ch_streaming_status = streaming_status;
        }

        return 0;
}

void homekit_camera_set_status(homekit_camera_t *camera, homekit_camera_streaming_status_t status) {
        camera->status = status;
        if (camera->ch_streaming_status) {
                // Notify subscribers with a freshly-built TLV value.
                homekit_characteristic_notify(
                        camera->ch_streaming_status,
                        HOMEKIT_TLV(build_streaming_status(status))
                        );
        }
}
