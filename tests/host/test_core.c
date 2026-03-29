#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "base64.h"
#include "bitset.h"
#include "query_params.h"

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

int main(void) {
    test_base64_roundtrip();
    test_query_params_iter();
    test_bitset_boundaries();

    puts("host tests passed");
    return 0;
}
