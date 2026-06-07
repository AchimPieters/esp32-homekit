// Host tests for the SRTP sender (src/srtp.c) and the H.264 RTP packetizer
// (src/rtp.c). Crypto primitives are checked against published vectors
// (FIPS-197 AES, RFC 2202 HMAC-SHA1, RFC 3711 B.3 SRTP KDF); the SRTP+RTP path
// is checked by a protect -> decrypt/verify -> NAL-reassembly round-trip.
//
// The .c files are #included so the test can reach the static primitives and
// so srtp/rtp live in a single translation unit (no duplicate public symbols).

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/srtp.c"
#include "../../src/rtp.c"

static int eq(const void *a, const void *b, int n) { return memcmp(a, b, n) == 0; }
static int from_hex(const char *h, uint8_t *o) {
        int n = 0;
        while (h[0] && h[1]) { unsigned v; sscanf(h, "%2x", &v); o[n++] = (uint8_t)v; h += 2; }
        return n;
}

static void test_vectors(void) {
        // AES-128 FIPS-197 known-answer
        uint8_t k[16], pt[16], ct[16], exp[16];
        from_hex("000102030405060708090a0b0c0d0e0f", k);
        from_hex("00112233445566778899aabbccddeeff", pt);
        from_hex("69c4e0d86a7b0430d8cdb78070b4c55a", exp);
        aes128_t a; aes128_init(&a, k); aes128_encrypt(&a, pt, ct);
        assert(eq(ct, exp, 16));

        // HMAC-SHA1 RFC 2202 #1 and #2
        uint8_t hk[20]; memset(hk, 0x0b, 20);
        uint8_t hexp[20], hout[20];
        from_hex("b617318655057264e28bc0b6fb378c8ef146be00", hexp);
        hmac_sha1(hk, 20, (const uint8_t *)"Hi There", 8, hout);
        assert(eq(hout, hexp, 20));
        from_hex("effcdf6ae5eb2fa2d27416d5f184df9c259a7c79", hexp);
        hmac_sha1((const uint8_t *)"Jefe", 4, (const uint8_t *)"what do ya want for nothing?", 28, hout);
        assert(eq(hout, hexp, 20));

        // SRTP KDF RFC 3711 B.3
        uint8_t mk[16], ms[14], ek[16], ak[20], sk[14], e[20];
        from_hex("E1F97A0D3E018BE0D64FA32C06DE4139", mk);
        from_hex("0EC675AD498AFEEBB6960B3AABE6", ms);
        srtp_kdf(mk, ms, 0, ek, 16); from_hex("C61E7A93744F39EE10734AFE3FF7A087", e); assert(eq(ek, e, 16));
        srtp_kdf(mk, ms, 1, ak, 20); from_hex("CEBE321F6FF7716B6FD4AB49AF256A156D38BAA4", e); assert(eq(ak, e, 20));
        srtp_kdf(mk, ms, 2, sk, 14); from_hex("30CBBC08863D8C85D49DB34A9AE1", e); assert(eq(sk, e, 14));
}

// Independent SRTP "unprotect" for the test: verify tag, decrypt payload.
static int srtp_unprotect(const uint8_t *mk, const uint8_t *ms, uint32_t ssrc,
                          uint8_t *pkt, size_t len, uint32_t roc) {
        if (len < 12 + HOMEKIT_SRTP_AUTH_TAG_SIZE) return -1;
        uint8_t enc[16], auth[20], salt[14];
        srtp_kdf(mk, ms, 0, enc, 16);
        srtp_kdf(mk, ms, 1, auth, 20);
        srtp_kdf(mk, ms, 2, salt, 14);
        size_t body = len - HOMEKIT_SRTP_AUTH_TAG_SIZE;
        // verify tag over body || roc
        uint8_t tmp[2048];
        assert(body + 4 <= sizeof(tmp));
        memcpy(tmp, pkt, body);
        tmp[body] = roc >> 24; tmp[body+1] = roc >> 16; tmp[body+2] = roc >> 8; tmp[body+3] = roc;
        uint8_t tag[20]; hmac_sha1(auth, 20, tmp, body + 4, tag);
        if (!eq(tag, pkt + body, HOMEKIT_SRTP_AUTH_TAG_SIZE)) return -2;
        // decrypt payload (bytes 12..body)
        uint16_t seq = (pkt[2] << 8) | pkt[3];
        uint8_t iv[16]; memset(iv, 0, 16);
        iv[4]=ssrc>>24; iv[5]=ssrc>>16; iv[6]=ssrc>>8; iv[7]=ssrc;
        uint64_t idx = ((uint64_t)roc << 16) | seq;
        iv[8]=idx>>40; iv[9]=idx>>32; iv[10]=idx>>24; iv[11]=idx>>16; iv[12]=idx>>8; iv[13]=idx;
        for (int i = 0; i < 14; i++) iv[i] ^= salt[i];
        aes128_t a; aes128_init(&a, enc);
        aes_cm_xor(&a, iv, pkt + 12, body - 12);
        return (int)body;
}

static void test_srtp_roundtrip(void) {
        uint8_t mk[16], ms[14];
        for (int i = 0; i < 16; i++) mk[i] = (uint8_t)(i * 3 + 1);
        for (int i = 0; i < 14; i++) ms[i] = (uint8_t)(i * 5 + 2);
        homekit_srtp_t ctx; assert(homekit_srtp_init(&ctx, mk, ms, 0xDEADBEEF) == 0);

        uint8_t pkt[256], orig[256];
        memset(pkt, 0, sizeof(pkt));
        pkt[0]=0x80; pkt[1]=99; pkt[2]=0; pkt[3]=0;  // seq 0
        pkt[8]=0xDE; pkt[9]=0xAD; pkt[10]=0xBE; pkt[11]=0xEF;
        for (int i = 0; i < 40; i++) pkt[12+i] = (uint8_t)(i ^ 0x5a);
        size_t plen = 12 + 40;
        memcpy(orig, pkt, plen);

        int n = homekit_srtp_protect(&ctx, pkt, plen, sizeof(pkt));
        assert(n == (int)plen + HOMEKIT_SRTP_AUTH_TAG_SIZE);
        assert(!eq(pkt + 12, orig + 12, 40));  // payload encrypted

        int b = srtp_unprotect(mk, ms, 0xDEADBEEF, pkt, n, 0);
        assert(b == (int)plen);
        assert(eq(pkt, orig, plen));           // decrypts back to original
}

// Capture sent packets for the RTP packetizer test.
#define MAXPKTS 64
static uint8_t cap_buf[MAXPKTS][2048];
static size_t  cap_len[MAXPKTS];
static int     cap_n;
static void capture_send(void *user, const uint8_t *pkt, size_t len) {
        (void)user; assert(cap_n < MAXPKTS); assert(len <= 2048);
        memcpy(cap_buf[cap_n], pkt, len); cap_len[cap_n] = len; cap_n++;
}

static void test_rtp_h264(void) {
        uint8_t mk[16], ms[14];
        for (int i = 0; i < 16; i++) mk[i] = (uint8_t)(i + 7);
        for (int i = 0; i < 14; i++) ms[i] = (uint8_t)(i + 11);

        // Access unit: small SPS NAL + large IDR NAL (forces FU-A), Annex-B.
        uint8_t au[4096]; size_t au_len = 0;
        const uint8_t sc4[4] = {0,0,0,1};
        // NAL1: type 7 (SPS), 8 bytes
        memcpy(au+au_len, sc4, 4); au_len += 4;
        size_t nal1_off = au_len;
        au[au_len++] = 0x67; for (int i = 0; i < 7; i++) au[au_len++] = (uint8_t)(0x10+i);
        size_t nal1_len = au_len - nal1_off;
        // NAL2: type 5 (IDR), 1500 bytes
        memcpy(au+au_len, sc4, 4); au_len += 4;
        size_t nal2_off = au_len;
        au[au_len++] = 0x65; for (int i = 0; i < 1499; i++) au[au_len++] = (uint8_t)(i*7+3);
        size_t nal2_len = au_len - nal2_off;

        uint8_t scratch[256];
        homekit_rtp_h264_t rc;
        assert(homekit_rtp_h264_init(&rc, mk, ms, 0x11223344, 99, sizeof(scratch),
                                     scratch, capture_send, NULL) == 0);
        cap_n = 0;
        int sent = homekit_rtp_h264_send(&rc, au, au_len, 90000);
        assert(sent == cap_n && sent >= 2);

        // Decrypt every packet and reassemble NAL units.
        static uint8_t reasm[8192]; size_t rlen = 0;
        uint8_t nals[8][4096]; size_t nlens[8]; int ncount = 0;
        uint32_t roc = 0; uint16_t prev = 0; int seen = 0;
        for (int i = 0; i < cap_n; i++) {
                uint8_t *p = cap_buf[i]; size_t L = cap_len[i];
                uint16_t seq = (p[2] << 8) | p[3];
                if (seen && seq < prev) roc++;
                prev = seq; seen = 1;
                int b = srtp_unprotect(mk, ms, 0x11223344, p, L, roc);
                assert(b > 12);
                const uint8_t *pl = p + 12; size_t pll = b - 12;
                uint8_t t = pl[0] & 0x1f;
                if (t == 28) { // FU-A
                        uint8_t fu = pl[1];
                        if (fu & 0x80) { // start
                                rlen = 0;
                                reasm[rlen++] = (pl[0] & 0xE0) | (fu & 0x1f); // reconstruct NAL header
                        }
                        memcpy(reasm + rlen, pl + 2, pll - 2); rlen += pll - 2;
                        if (fu & 0x40) { // end
                                memcpy(nals[ncount], reasm, rlen); nlens[ncount] = rlen; ncount++;
                        }
                } else { // single NAL
                        memcpy(nals[ncount], pl, pll); nlens[ncount] = pll; ncount++;
                }
        }
        assert(ncount == 2);
        assert(nlens[0] == nal1_len && eq(nals[0], au + nal1_off, nal1_len));
        assert(nlens[1] == nal2_len && eq(nals[1], au + nal2_off, nal2_len));

        // marker bit must be set only on the final packet of the access unit.
        for (int i = 0; i < cap_n; i++) {
                int marker = (cap_buf[i][1] & 0x80) != 0;
                assert(marker == (i == cap_n - 1));
        }
}

int main(void) {
        test_vectors();
        test_srtp_roundtrip();
        test_rtp_h264();
        puts("srtp/rtp tests passed");
        return 0;
}
