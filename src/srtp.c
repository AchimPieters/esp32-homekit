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

#include <homekit/srtp.h>
#include <string.h>

static const uint8_t SBOX[256] = {
0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16};

static const uint8_t RCON[11] = {0x00,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1b,0x36};

typedef struct { uint8_t rk[176]; } aes128_t;  // 11 round keys * 16 bytes

static void aes128_init(aes128_t *a, const uint8_t key[16]) {
        memcpy(a->rk, key, 16);
        for (int i = 4; i < 44; i++) {
                uint8_t t[4];
                memcpy(t, a->rk + (i-1)*4, 4);
                if (i % 4 == 0) {
                        uint8_t tmp = t[0];
                        t[0] = SBOX[t[1]] ^ RCON[i/4];
                        t[1] = SBOX[t[2]];
                        t[2] = SBOX[t[3]];
                        t[3] = SBOX[tmp];
                }
                for (int j = 0; j < 4; j++)
                        a->rk[i*4+j] = a->rk[(i-4)*4+j] ^ t[j];
        }
}

static uint8_t xtime(uint8_t x) { return (uint8_t)((x<<1) ^ ((x>>7)*0x1b)); }
static uint8_t mul(uint8_t x, uint8_t y) {
        uint8_t r = 0;
        while (y) { if (y&1) r ^= x; x = xtime(x); y >>= 1; }
        return r;
}

static void aes128_encrypt(const aes128_t *a, const uint8_t in[16], uint8_t out[16]) {
        uint8_t s[16];
        for (int i = 0; i < 16; i++) s[i] = in[i] ^ a->rk[i];
        for (int round = 1; round <= 10; round++) {
                uint8_t t[16];
                for (int i = 0; i < 16; i++) t[i] = SBOX[s[i]];
                // ShiftRows
                uint8_t sr[16] = {
                  t[0],  t[5],  t[10], t[15],
                  t[4],  t[9],  t[14], t[3],
                  t[8],  t[13], t[2],  t[7],
                  t[12], t[1],  t[6],  t[11]};
                if (round < 10) {
                        for (int c = 0; c < 4; c++) {
                                uint8_t *col = sr + c*4;
                                uint8_t a0=col[0],a1=col[1],a2=col[2],a3=col[3];
                                col[0]=mul(a0,2)^mul(a1,3)^a2^a3;
                                col[1]=a0^mul(a1,2)^mul(a2,3)^a3;
                                col[2]=a0^a1^mul(a2,2)^mul(a3,3);
                                col[3]=mul(a0,3)^a1^a2^mul(a3,2);
                        }
                }
                for (int i = 0; i < 16; i++) s[i] = sr[i] ^ a->rk[round*16+i];
        }
        memcpy(out, s, 16);
}

// ---------------- SHA-1 ------------------------------------------------------
typedef struct { uint32_t h[5]; uint64_t len; uint8_t buf[64]; size_t n; } sha1_t;
static uint32_t rol32(uint32_t x, int c){ return (x<<c)|(x>>(32-c)); }
static void sha1_block(sha1_t *s, const uint8_t *p) {
        uint32_t w[80];
        for (int i=0;i<16;i++) w[i]=((uint32_t)p[i*4]<<24)|((uint32_t)p[i*4+1]<<16)|((uint32_t)p[i*4+2]<<8)|(uint32_t)p[i*4+3];
        for (int i=16;i<80;i++) w[i]=rol32(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
        uint32_t a=s->h[0],b=s->h[1],c=s->h[2],d=s->h[3],e=s->h[4];
        for (int i=0;i<80;i++){
                uint32_t f,k;
                if(i<20){f=(b&c)|((~b)&d);k=0x5A827999;}
                else if(i<40){f=b^c^d;k=0x6ED9EBA1;}
                else if(i<60){f=(b&c)|(b&d)|(c&d);k=0x8F1BBCDC;}
                else {f=b^c^d;k=0xCA62C1D6;}
                uint32_t t=rol32(a,5)+f+e+k+w[i];
                e=d;d=c;c=rol32(b,30);b=a;a=t;
        }
        s->h[0]+=a;s->h[1]+=b;s->h[2]+=c;s->h[3]+=d;s->h[4]+=e;
}
static void sha1_init(sha1_t *s){ s->h[0]=0x67452301;s->h[1]=0xEFCDAB89;s->h[2]=0x98BADCFE;s->h[3]=0x10325476;s->h[4]=0xC3D2E1F0;s->len=0;s->n=0; }
static void sha1_update(sha1_t *s, const uint8_t *p, size_t len){
        s->len += len;
        while (len){ size_t take=64-s->n; if(take>len)take=len; memcpy(s->buf+s->n,p,take); s->n+=take; p+=take; len-=take;
                if(s->n==64){ sha1_block(s,s->buf); s->n=0; } }
}
static void sha1_final(sha1_t *s, uint8_t out[20]){
        uint64_t bits = s->len*8;
        uint8_t pad=0x80; sha1_update(s,&pad,1);
        uint8_t z=0; while(s->n!=56) sha1_update(s,&z,1);
        uint8_t lb[8]; for(int i=0;i<8;i++) lb[i]=(uint8_t)(bits>>(56-i*8)); sha1_update(s,lb,8);
        for(int i=0;i<5;i++){ out[i*4]=s->h[i]>>24; out[i*4+1]=s->h[i]>>16; out[i*4+2]=s->h[i]>>8; out[i*4+3]=s->h[i]; }
}

// ---------------- HMAC-SHA1 -------------------------------------------------
static void hmac_sha1(const uint8_t *key, size_t keylen, const uint8_t *msg, size_t msglen, uint8_t out[20]){
        uint8_t k[64]; memset(k,0,64);
        if (keylen>64){ sha1_t t; sha1_init(&t); sha1_update(&t,key,keylen); sha1_final(&t,k); }
        else memcpy(k,key,keylen);
        uint8_t ipad[64],opad[64];
        for(int i=0;i<64;i++){ ipad[i]=k[i]^0x36; opad[i]=k[i]^0x5c; }
        uint8_t inner[20];
        sha1_t s; sha1_init(&s); sha1_update(&s,ipad,64); sha1_update(&s,msg,msglen); sha1_final(&s,inner);
        sha1_t o; sha1_init(&o); sha1_update(&o,opad,64); sha1_update(&o,inner,20); sha1_final(&o,out);
}

// expose for tests

// ---------------- AES-CM keystream ------------------------------------------
static void aes_cm(const aes128_t *ek, const uint8_t iv[16], uint8_t *out, size_t len) {
        uint8_t ctr[16]; memcpy(ctr, iv, 16);
        uint8_t blk[16]; size_t off = 0;
        while (off < len) {
                aes128_encrypt(ek, ctr, blk);
                size_t n = len - off; if (n > 16) n = 16;
                memcpy(out + off, blk, n); off += n;
                for (int b = 15; b >= 0; b--) { if (++ctr[b]) break; }
        }
}

// AES-CM keystream XORed into data (in place).
static void aes_cm_xor(const aes128_t *ek, const uint8_t iv[16], uint8_t *data, size_t len) {
        uint8_t ctr[16]; memcpy(ctr, iv, 16);
        uint8_t blk[16]; size_t off = 0;
        while (off < len) {
                aes128_encrypt(ek, ctr, blk);
                size_t n = len - off; if (n > 16) n = 16;
                for (size_t i = 0; i < n; i++) data[off + i] ^= blk[i];
                off += n;
                for (int b = 15; b >= 0; b--) { if (++ctr[b]) break; }
        }
}

// SRTP key derivation (RFC 3711 4.3, key_derivation_rate = 0).
// label: 0 = encryption, 1 = authentication, 2 = salt.
static void srtp_kdf(const uint8_t master_key[16], const uint8_t master_salt[14],
                     uint8_t label, uint8_t *out, size_t len) {
        aes128_t ek; aes128_init(&ek, master_key);
        uint8_t iv[16]; memcpy(iv, master_salt, 14); iv[14] = 0; iv[15] = 0;
        iv[7] ^= label;
        aes_cm(&ek, iv, out, len);
}

int homekit_srtp_init(homekit_srtp_t *ctx,
                      const uint8_t master_key[16], const uint8_t master_salt[14],
                      uint32_t ssrc) {
        if (!ctx || !master_key || !master_salt) return -1;
        uint8_t enc_key[16];
        srtp_kdf(master_key, master_salt, 0, enc_key, 16);
        srtp_kdf(master_key, master_salt, 1, ctx->auth_key, 20);
        srtp_kdf(master_key, master_salt, 2, ctx->salt, 14);

        aes128_t ek; aes128_init(&ek, enc_key);
        memcpy(ctx->enc_rk, ek.rk, 176);
        memset(enc_key, 0, sizeof(enc_key));

        ctx->ssrc = ssrc;
        ctx->roc = 0;
        ctx->last_seq = 0;
        ctx->seq_seen = 0;
        return 0;
}

int homekit_srtp_protect(homekit_srtp_t *ctx, uint8_t *pkt, size_t pkt_len, size_t capacity) {
        if (!ctx || !pkt || pkt_len < 12) return -1;
        if (capacity < pkt_len + HOMEKIT_SRTP_AUTH_TAG_SIZE) return -2;

        uint16_t seq = ((uint16_t)pkt[2] << 8) | pkt[3];

        // Maintain the 32-bit rollover counter from the 16-bit sequence number.
        if (ctx->seq_seen && seq < ctx->last_seq)
                ctx->roc++;
        ctx->last_seq = seq;
        ctx->seq_seen = 1;

        // AES-CM IV (RFC 3711 4.1.1):
        //   iv = (salt||0x0000) XOR (ssrc<<64) XOR (packet_index<<16)
        //   packet_index = ROC(32) || SEQ(16)  (48-bit)
        uint8_t iv[16];
        memset(iv, 0, 16);
        iv[4] = (uint8_t)(ctx->ssrc >> 24); iv[5] = (uint8_t)(ctx->ssrc >> 16);
        iv[6] = (uint8_t)(ctx->ssrc >> 8);  iv[7] = (uint8_t)(ctx->ssrc);
        uint64_t index = ((uint64_t)ctx->roc << 16) | seq;
        iv[8]  = (uint8_t)(index >> 40); iv[9]  = (uint8_t)(index >> 32);
        iv[10] = (uint8_t)(index >> 24); iv[11] = (uint8_t)(index >> 16);
        iv[12] = (uint8_t)(index >> 8);  iv[13] = (uint8_t)(index);
        for (int i = 0; i < 14; i++) iv[i] ^= ctx->salt[i];

        // Encrypt the payload (everything after the 12-byte header).
        aes128_t ek; memcpy(ek.rk, ctx->enc_rk, 176);
        aes_cm_xor(&ek, iv, pkt + 12, pkt_len - 12);

        // Authenticate the whole (encrypted) packet plus the 32-bit ROC.
        // The SRTP authenticated portion is the RTP packet followed by the ROC.
        // Place the ROC just past the packet (there is room: capacity already
        // reserves >= 10 bytes for the tag) to form a contiguous HMAC input,
        // then overwrite those bytes with the 10-byte tag.
        pkt[pkt_len + 0] = (uint8_t)(ctx->roc >> 24);
        pkt[pkt_len + 1] = (uint8_t)(ctx->roc >> 16);
        pkt[pkt_len + 2] = (uint8_t)(ctx->roc >> 8);
        pkt[pkt_len + 3] = (uint8_t)(ctx->roc);
        uint8_t tag[20];
        hmac_sha1(ctx->auth_key, 20, pkt, pkt_len + 4, tag);
        memcpy(pkt + pkt_len, tag, HOMEKIT_SRTP_AUTH_TAG_SIZE);
        return (int)(pkt_len + HOMEKIT_SRTP_AUTH_TAG_SIZE);
}
