#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "base64.h"
#include "bitset.h"
#include "query_params.h"
#include "mdns_name.h"
#include <homekit/tlv.h>

static void test_base64_roundtrip(void) {
    const unsigned char input[] = "HomeKit-ESP32";
    unsigned char encoded[64] = {0};
    unsigned char decoded[64] = {0};

    int encoded_len = base64_encode(input, strlen((const char *)input), encoded);
    assert(encoded_len > 0);
    assert((size_t)encoded_len == base64_encoded_size(input, strlen((const char *)input)));

    int decoded_len = base64_decode(encoded, encoded_len, decoded);
    assert(decoded_len == (int)strlen((const char *)input));
    assert(memcmp(decoded, input, strlen((const char *)input)) == 0);
}

static void test_base64_rejects_invalid(void) {
    unsigned char out[64] = {0};

    // Non-multiple-of-4 length is rejected.
    assert(base64_decode((const unsigned char *)"abc", 3, out) == -1);

    // Illegal characters are rejected instead of silently decoding to 0.
    assert(base64_decode((const unsigned char *)"ab*d", 4, out) == -1);
    assert(base64_decode((const unsigned char *)"   =", 4, out) == -1);

    // Misplaced padding is rejected.
    assert(base64_decode((const unsigned char *)"a=bc", 4, out) == -1);
    assert(base64_decode((const unsigned char *)"ab=c", 4, out) == -1);

    // A well-formed padded block still decodes.
    assert(base64_decode((const unsigned char *)"QQ==", 4, out) == 1);
    assert(out[0] == 'A');
}

static void test_query_params_iter(void) {
    const char *query = "foo=bar&empty=&flag#fragment";
    query_param_iterator_t it;
    query_param_t p;

    assert(query_param_iterator_init(&it, query, strlen(query)) == 0);

    assert(query_param_iterator_next(&it, &p));
    assert(p.name_len == 3 && strncmp(p.name, "foo", p.name_len) == 0);
    assert(p.value_len == 3 && strncmp(p.value, "bar", p.value_len) == 0);

    assert(query_param_iterator_next(&it, &p));
    assert(p.name_len == 5 && strncmp(p.name, "empty", p.name_len) == 0);
    assert(p.value == NULL && p.value_len == 0);

    assert(query_param_iterator_next(&it, &p));
    assert(p.name_len == 4 && strncmp(p.name, "flag", p.name_len) == 0);
    assert(p.value == NULL && p.value_len == 0);

    assert(!query_param_iterator_next(&it, &p));
    query_param_iterator_done(&it);
}

static void test_bitset_boundaries(void) {
    bitset_t *bs = bitset_new(128);
    assert(bs != NULL);

    for (uint16_t i = 0; i < 128; i++) {
        assert(!bitset_isset(bs, i));
        bitset_set(bs, i);
        assert(bitset_isset(bs, i));
    }

    bitset_set(bs, 128);
    assert(!bitset_isset(bs, 128));

    for (uint16_t i = 0; i < 128; i++) {
        bitset_clear(bs, i);
        assert(!bitset_isset(bs, i));
    }

    bitset_free(bs);
}

static void test_tlv_parse_valid(void) {
    // type=6, len=1, value=2 ; type=1, len=3, value="abc"
    const unsigned char buf[] = {6, 1, 2, 1, 3, 'a', 'b', 'c'};
    tlv_values_t *values = tlv_new();
    assert(values != NULL);

    assert(tlv_parse(buf, sizeof(buf), values) == 0);

    tlv_t *state = tlv_get_value(values, 6);
    assert(state && state->size == 1 && state->value[0] == 2);

    tlv_t *id = tlv_get_value(values, 1);
    assert(id && id->size == 3 && memcmp(id->value, "abc", 3) == 0);

    tlv_free(values);
}

static void test_tlv_parse_rejects_malformed(void) {
    // Length byte claims 5 bytes of value but only 2 are present: must not
    // read past the buffer (ASan would catch an over-read) and must error out.
    {
        const unsigned char buf[] = {6, 5, 1, 2};
        tlv_values_t *values = tlv_new();
        assert(values != NULL);
        assert(tlv_parse(buf, sizeof(buf), values) != 0);
        tlv_free(values);
    }

    // Dangling type byte with no length byte.
    {
        const unsigned char buf[] = {6, 1, 2, 7};
        tlv_values_t *values = tlv_new();
        assert(values != NULL);
        assert(tlv_parse(buf, sizeof(buf), values) != 0);
        tlv_free(values);
    }

    // A 255-length chunk that is not actually followed by 255 bytes.
    {
        unsigned char buf[4] = {3, 255, 0, 0};
        tlv_values_t *values = tlv_new();
        assert(values != NULL);
        assert(tlv_parse(buf, sizeof(buf), values) != 0);
        tlv_free(values);
    }
}

static void test_mdns_name_parsing(void) {
    // "local" label followed by root.
    uint8_t local[] = {5, 'l','o','c','a','l', 0};
    assert(mdns_match_label(local, sizeof(local), 0, "local", 5) == 6);
    assert(mdns_match_label(local, sizeof(local), 0, "wrong", 5) == -1);
    assert(mdns_match_label(local, sizeof(local), 0, "loc", 3) == -1); // length mismatch

    // Truncated: length byte claims 5 but buffer is too short.
    uint8_t trunc[] = {5, 'l','o','c'};
    assert(mdns_match_label(trunc, sizeof(trunc), 0, "local", 5) == -1);

    // Valid compression pointer: offset 0 -> points to offset 4 ("local").
    uint8_t ptr[] = {0xC0, 0x04, 0x00, 0x00, 5, 'l','o','c','a','l'};
    assert(mdns_match_label(ptr, sizeof(ptr), 0, "local", 5) == 10);

    // Cyclic compression pointers must terminate (not hang) and report error.
    uint8_t cyc[] = {0xC0, 0x02, 0xC0, 0x00};
    assert(mdns_match_label(cyc, sizeof(cyc), 0, "local", 5) == -1);

    // skip_name: normal name, pointer, and truncated.
    uint8_t name[] = {5, 'l','o','c','a','l', 0};
    assert(mdns_skip_name(name, sizeof(name), 0) == 7);
    uint8_t pname[] = {0xC0, 0x00};
    assert(mdns_skip_name(pname, sizeof(pname), 0) == 2);
    uint8_t tname[] = {5, 'a', 'b'};
    assert(mdns_skip_name(tname, sizeof(tname), 0) == -1);
}

int main(void) {
    test_base64_roundtrip();
    test_base64_rejects_invalid();
    test_query_params_iter();
    test_bitset_boundaries();
    test_tlv_parse_valid();
    test_tlv_parse_rejects_malformed();
    test_mdns_name_parsing();

    puts("host tests passed");
    return 0;
}
