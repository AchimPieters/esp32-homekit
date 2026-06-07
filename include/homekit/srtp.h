#ifndef __HOMEKIT_SRTP_H__
#define __HOMEKIT_SRTP_H__

// Minimal SRTP sender for HomeKit camera streaming.
// Crypto suite: SRTP_AES_CM_128_HMAC_SHA1_80 (the mandatory HomeKit suite).
//
// Self-contained (AES-128 / SHA-1 / HMAC-SHA1 implemented in srtp.c), so it is
// portable and host-testable against published test vectors (FIPS-197,
// RFC 2202, RFC 3711). It only implements the sender side (protect); that is
// all an accessory needs to stream video/audio to a controller.

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HOMEKIT_SRTP_MASTER_KEY_SIZE   16
#define HOMEKIT_SRTP_MASTER_SALT_SIZE  14
#define HOMEKIT_SRTP_AUTH_TAG_SIZE     10   // 80-bit tag

typedef struct {
        uint8_t  enc_rk[176];   // expanded AES-128 session encryption round keys
        uint8_t  auth_key[20];  // session HMAC-SHA1 key
        uint8_t  salt[14];      // session salt
        uint32_t ssrc;
        uint32_t roc;           // rollover counter
        uint16_t last_seq;
        int      seq_seen;
} homekit_srtp_t;

// Initialize the SRTP context for one outgoing stream identified by ssrc.
// master_key/master_salt come from the negotiated HomeKit camera session.
// Returns 0 on success.
int homekit_srtp_init(homekit_srtp_t *ctx,
                      const uint8_t master_key[HOMEKIT_SRTP_MASTER_KEY_SIZE],
                      const uint8_t master_salt[HOMEKIT_SRTP_MASTER_SALT_SIZE],
                      uint32_t ssrc);

// Protect a complete RTP packet in place: encrypts the payload (AES-CM) and
// appends a 10-byte HMAC-SHA1-80 authentication tag.
//   pkt      : buffer holding a 12-byte RTP header followed by the payload
//   pkt_len  : current packet length (header + payload), >= 12
//   capacity : size of pkt buffer; must be >= pkt_len + 10
// The packet's SSRC field (bytes 8..11) must equal ctx->ssrc.
// Returns the new total length (pkt_len + 10) on success, or < 0 on error.
int homekit_srtp_protect(homekit_srtp_t *ctx, uint8_t *pkt, size_t pkt_len, size_t capacity);

#ifdef __cplusplus
}
#endif

#endif
