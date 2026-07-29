// Host regression tests for the HomeKit camera protocol layer (src/camera.c).
// Platform dependencies are stubbed so the TLV state machine can be exercised
// on the host under ASan/UBSan.

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <homekit/camera.h>
#include <homekit/tlv.h>

// ---- stubs for platform symbols referenced by camera.c ----
void homekit_characteristic_notify(homekit_characteristic_t *ch, const homekit_value_t value) {
    (void)ch;
    // The server would free a non-static tlv value; mirror that so ASan is happy.
    homekit_value_t v = value;
    homekit_value_destruct(&v);
}
uint32_t homekit_random(void) { return 0xA5A5A5A5u; }
void homekit_random_fill(uint8_t *data, size_t size) {
    for (size_t i = 0; i < size; i++) data[i] = (uint8_t)(i * 7 + 3);
}

static int start_calls = 0;
static int end_calls = 0;
static homekit_camera_session_t last_started;

static bool prepare_endpoints(homekit_camera_t *c, homekit_camera_session_t *s) {
    (void)c;
    s->accessory.ip_version = 0;
    snprintf(s->accessory.ip_address, sizeof(s->accessory.ip_address), "192.168.1.50");
    s->accessory.video_port = 6000;
    s->accessory.audio_port = 6002;
    return true;
}
static void on_start(homekit_camera_t *c, const homekit_camera_session_t *s) {
    (void)c; start_calls++; last_started = *s;
}
static void on_end(homekit_camera_t *c, const uint8_t id[16]) { (void)c; (void)id; end_calls++; }

static homekit_camera_config_t make_config(void) {
    homekit_camera_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.h264_params[0] = (homekit_camera_h264_param_t){
        homekit_camera_h264_profile_main, homekit_camera_h264_level_4_0};
    cfg.h264_param_count = 1;
    cfg.attributes[0] = (homekit_camera_video_attributes_t){1920, 1080, 30};
    cfg.attributes[1] = (homekit_camera_video_attributes_t){1280, 720, 30};
    cfg.attribute_count = 2;
    cfg.audio[0] = (homekit_camera_audio_config_t){
        homekit_camera_audio_codec_opus, 1, homekit_camera_audio_sample_rate_16khz, 20, true};
    cfg.audio_count = 1;
    cfg.srtp_suites[0] = homekit_camera_srtp_crypto_aes_cm_128_hmac_sha1_80;
    cfg.srtp_suite_count = 1;
    return cfg;
}

static homekit_characteristic_t mk_char(void) {
    homekit_characteristic_t ch;
    memset(&ch, 0, sizeof(ch));
    ch.format = homekit_format_tlv;
    return ch;
}

static void test_supported_config(void) {
    homekit_camera_config_t cfg = make_config();
    homekit_camera_t *cam = homekit_camera_new(&cfg, NULL, NULL);
    assert(cam);

    homekit_characteristic_t sv = mk_char(), sa = mk_char(), sr = mk_char(),
                             se = mk_char(), sc = mk_char(), ss = mk_char();
    assert(homekit_camera_bind(cam, &sv, &sa, &sr, &se, &sc, &ss) == 0);

    // Supported video must contain a codec config (type 1) that decodes and
    // carries two video attributes.
    homekit_value_t v = sv.getter_ex(&sv);
    assert(v.format == homekit_format_tlv && v.tlv_values);
    tlv_values_t *codec = tlv_get_tlv_value(v.tlv_values, 1);
    assert(codec);
    int attr_count = 0;
    for (tlv_t *t = codec->head; t; t = t->next)
        if (t->type == 3 /* attributes */) attr_count++;
    assert(attr_count == 2);
    tlv_free(codec);
    homekit_value_destruct(&v);

    // Supported RTP advertises the AES-128 suite.
    homekit_value_t r = sr.getter_ex(&sr);
    assert(tlv_get_integer_value(r.tlv_values, 2, -1) ==
           homekit_camera_srtp_crypto_aes_cm_128_hmac_sha1_80);
    homekit_value_destruct(&r);

    homekit_camera_free(cam);
}

static void test_setup_endpoints_roundtrip(void) {
    homekit_camera_config_t cfg = make_config();
    homekit_camera_callbacks_t cb = {0};
    cb.prepare_endpoints = prepare_endpoints;
    homekit_camera_t *cam = homekit_camera_new(&cfg, &cb, NULL);
    assert(cam);

    homekit_characteristic_t sv = mk_char(), sa = mk_char(), sr = mk_char(),
                             se = mk_char(), sc = mk_char(), ss = mk_char();
    assert(homekit_camera_bind(cam, &sv, &sa, &sr, &se, &sc, &ss) == 0);

    // Build a controller Setup Endpoints request.
    tlv_values_t *req = tlv_new();
    uint8_t sid[16];
    for (int i = 0; i < 16; i++) sid[i] = (uint8_t)i;
    tlv_add_value(req, 1 /* session id */, sid, sizeof(sid));

    tlv_values_t *addr = tlv_new();
    tlv_add_integer_value(addr, 1, 1, 0);            // IPv4
    tlv_add_string_value(addr, 2, "10.0.0.9");       // controller IP
    tlv_add_integer_value(addr, 3, 2, 5000);         // video port
    tlv_add_integer_value(addr, 4, 2, 5002);         // audio port
    tlv_add_tlv_value(req, 3 /* address */, addr);
    tlv_free(addr);

    uint8_t key[16], salt[14];
    memset(key, 0xAB, sizeof(key));
    memset(salt, 0xCD, sizeof(salt));
    tlv_values_t *vsrtp = tlv_new();
    tlv_add_integer_value(vsrtp, 1, 1, 0);           // AES-128
    tlv_add_value(vsrtp, 2, key, sizeof(key));
    tlv_add_value(vsrtp, 3, salt, sizeof(salt));
    tlv_add_tlv_value(req, 4 /* srtp video */, vsrtp);
    tlv_free(vsrtp);

    se.setter_ex(&se, HOMEKIT_TLV(req));
    homekit_value_t reqv = HOMEKIT_TLV(req);
    homekit_value_destruct(&reqv);

    // Read back the accessory response.
    homekit_value_t resp = se.getter_ex(&se);
    assert(resp.format == homekit_format_tlv && resp.tlv_values);

    tlv_t *rsid = tlv_get_value(resp.tlv_values, 1);
    assert(rsid && rsid->size == 16 && memcmp(rsid->value, sid, 16) == 0); // echoed
    assert(tlv_get_integer_value(resp.tlv_values, 2, -1) == 0);            // status success

    tlv_values_t *raddr = tlv_get_tlv_value(resp.tlv_values, 3);
    assert(raddr);
    tlv_t *ip = tlv_get_value(raddr, 2);
    assert(ip && ip->size == strlen("192.168.1.50") &&
           memcmp(ip->value, "192.168.1.50", ip->size) == 0);             // from prepare cb
    assert(tlv_get_integer_value(raddr, 3, 0) == 6000);
    tlv_free(raddr);

    tlv_values_t *rv = tlv_get_tlv_value(resp.tlv_values, 4);
    assert(rv);
    tlv_t *rkey = tlv_get_value(rv, 2);
    assert(rkey && rkey->size == 16); // accessory generated a 16-byte AES-128 key
    tlv_free(rv);

    assert(tlv_get_value(resp.tlv_values, 6)); // video SSRC present
    homekit_value_destruct(&resp);

    homekit_camera_free(cam);
}

static void test_selected_config_start(void) {
    homekit_camera_config_t cfg = make_config();
    homekit_camera_callbacks_t cb = {0};
    cb.on_stream_start = on_start;
    cb.on_stream_end = on_end;
    homekit_camera_t *cam = homekit_camera_new(&cfg, &cb, NULL);
    assert(cam);

    homekit_characteristic_t sv = mk_char(), sa = mk_char(), sr = mk_char(),
                             se = mk_char(), sc = mk_char(), ss = mk_char();
    assert(homekit_camera_bind(cam, &sv, &sa, &sr, &se, &sc, &ss) == 0);

    start_calls = end_calls = 0;

    // Selected RTP Stream Configuration: Start
    tlv_values_t *sel = tlv_new();
    tlv_values_t *ctrl = tlv_new();
    uint8_t sid[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    tlv_add_value(ctrl, 1 /* session id */, sid, sizeof(sid));
    tlv_add_integer_value(ctrl, 2 /* command */, 1, homekit_camera_command_start);
    tlv_add_tlv_value(sel, 1 /* session control */, ctrl);
    tlv_free(ctrl);

    tlv_values_t *vp = tlv_new();
    tlv_values_t *attr = tlv_new();
    tlv_add_integer_value(attr, 1, 2, 1280);
    tlv_add_integer_value(attr, 2, 2, 720);
    tlv_add_integer_value(attr, 3, 1, 30);
    tlv_add_tlv_value(vp, 3 /* attributes */, attr);
    tlv_free(attr);
    tlv_add_tlv_value(sel, 2 /* video params */, vp);
    tlv_free(vp);

    sc.setter_ex(&sc, HOMEKIT_TLV(sel));
    homekit_value_t selv = HOMEKIT_TLV(sel);
    homekit_value_destruct(&selv);

    assert(start_calls == 1);
    assert(cam->session_active);
    assert(cam->status == homekit_camera_streaming_status_in_use);
    assert(last_started.selected_video.width == 1280 && last_started.selected_video.height == 720);

    // End
    tlv_values_t *sel2 = tlv_new();
    tlv_values_t *ctrl2 = tlv_new();
    tlv_add_value(ctrl2, 1, sid, sizeof(sid));
    tlv_add_integer_value(ctrl2, 2, 1, homekit_camera_command_end);
    tlv_add_tlv_value(sel2, 1, ctrl2);
    tlv_free(ctrl2);
    sc.setter_ex(&sc, HOMEKIT_TLV(sel2));
    homekit_value_t sel2v = HOMEKIT_TLV(sel2);
    homekit_value_destruct(&sel2v);

    assert(end_calls == 1);
    assert(!cam->session_active);
    assert(cam->status == homekit_camera_streaming_status_available);

    homekit_camera_free(cam);
}

int main(void) {
    test_supported_config();
    test_setup_endpoints_roundtrip();
    test_selected_config_start();
    puts("camera tests passed");
    return 0;
}
