#ifndef __HOMEKIT_RTP_H__
#define __HOMEKIT_RTP_H__

// H.264 RTP packetizer (RFC 6184) with SRTP protection, for sending an
// accessory's encoded video to a HomeKit controller. Takes an H.264 access
// unit in Annex-B byte-stream form (NAL units separated by 0x000001 / 0x00000001
// start codes, as produced by esp_h264), splits it into RTP packets
// (single-NAL or FU-A fragmentation to respect the MTU), SRTP-protects each
// packet, and hands it to a send callback (typically a UDP sendto()).

#include <stdint.h>
#include <stddef.h>
#include <homekit/srtp.h>

#ifdef __cplusplus
extern "C" {
#endif

// Called for every fully-built SRTP packet. `pkt`/`len` are owned by the
// packetizer (valid only during the call); copy if needed.
typedef void (*homekit_rtp_send_fn)(void *user, const uint8_t *pkt, size_t len);

typedef struct {
        homekit_srtp_t      srtp;
        uint32_t            ssrc;
        uint8_t             payload_type;   // dynamic PT negotiated for the stream
        uint16_t            seq;
        size_t              mtu;            // max SRTP packet size (e.g. 1378)
        homekit_rtp_send_fn send;
        void               *user;
        uint8_t            *buf;            // scratch buffer of `mtu` bytes
} homekit_rtp_h264_t;

// Initialize. `buf` must point to at least `mtu` bytes of scratch storage that
// outlives the packetizer. master_key/master_salt + ssrc come from the
// negotiated camera session; payload_type from the Selected RTP config.
// Returns 0 on success.
int homekit_rtp_h264_init(homekit_rtp_h264_t *ctx,
                          const uint8_t master_key[16], const uint8_t master_salt[14],
                          uint32_t ssrc, uint8_t payload_type, size_t mtu,
                          uint8_t *buf,
                          homekit_rtp_send_fn send, void *user);

// Packetize and send one Annex-B access unit at the given 90 kHz timestamp.
// Returns the number of RTP packets sent, or < 0 on error.
int homekit_rtp_h264_send(homekit_rtp_h264_t *ctx,
                          const uint8_t *au, size_t au_len, uint32_t timestamp);

#ifdef __cplusplus
}
#endif

#endif
