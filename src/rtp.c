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

#include <homekit/rtp.h>
#include <string.h>

#define RTP_HEADER_SIZE 12
#define FU_A_TYPE 28

int homekit_rtp_h264_init(homekit_rtp_h264_t *ctx,
                          const uint8_t master_key[16], const uint8_t master_salt[14],
                          uint32_t ssrc, uint8_t payload_type, size_t mtu,
                          uint8_t *buf,
                          homekit_rtp_send_fn send, void *user) {
        if (!ctx || !buf || !send) return -1;
        // Need room for RTP header + at least 2 bytes payload + SRTP tag.
        if (mtu < RTP_HEADER_SIZE + 2 + HOMEKIT_SRTP_AUTH_TAG_SIZE) return -1;
        if (homekit_srtp_init(&ctx->srtp, master_key, master_salt, ssrc)) return -1;
        ctx->ssrc = ssrc;
        ctx->payload_type = payload_type;
        ctx->seq = 0;
        ctx->mtu = mtu;
        ctx->buf = buf;
        ctx->send = send;
        ctx->user = user;
        return 0;
}

// Find the next Annex-B NAL unit in [p, end). Sets *nal/*nal_len to the NAL
// (without start code). Returns the pointer just past it, or NULL when done.
static const uint8_t *next_nal(const uint8_t *p, const uint8_t *end,
                               const uint8_t **nal, size_t *nal_len) {
        // skip to a start code (00 00 01 or 00 00 00 01)
        while (p + 3 <= end) {
                if (p[0] == 0 && p[1] == 0 && p[2] == 1) { p += 3; break; }
                if (p + 4 <= end && p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 1) { p += 4; break; }
                p++;
        }
        if (p >= end) return NULL;
        const uint8_t *start = p;
        // find the next start code
        while (p + 3 <= end) {
                if (p[0] == 0 && p[1] == 0 && p[2] == 1) break;
                p++;
        }
        const uint8_t *stop = (p + 3 <= end) ? p : end;
        // a 4-byte start code's leading zero belongs to the prefix, not this NAL
        if (stop > start && stop != end && stop[-1] == 0) stop--;
        *nal = start;
        *nal_len = (size_t)(stop - start);
        return stop;
}

// Build, protect and send one RTP packet from `payload`. marker sets the M bit.
static int emit_packet(homekit_rtp_h264_t *ctx, const uint8_t *payload, size_t plen,
                       uint32_t ts, int marker) {
        uint8_t *pkt = ctx->buf;
        size_t cap = ctx->mtu;
        if (RTP_HEADER_SIZE + plen + HOMEKIT_SRTP_AUTH_TAG_SIZE > cap) return -1;

        pkt[0] = 0x80;                                  // V=2, P=0, X=0, CC=0
        pkt[1] = (uint8_t)((marker ? 0x80 : 0) | (ctx->payload_type & 0x7f));
        pkt[2] = (uint8_t)(ctx->seq >> 8);
        pkt[3] = (uint8_t)(ctx->seq);
        pkt[4] = (uint8_t)(ts >> 24); pkt[5] = (uint8_t)(ts >> 16);
        pkt[6] = (uint8_t)(ts >> 8);  pkt[7] = (uint8_t)(ts);
        pkt[8]  = (uint8_t)(ctx->ssrc >> 24); pkt[9]  = (uint8_t)(ctx->ssrc >> 16);
        pkt[10] = (uint8_t)(ctx->ssrc >> 8);  pkt[11] = (uint8_t)(ctx->ssrc);
        memcpy(pkt + RTP_HEADER_SIZE, payload, plen);
        ctx->seq++;

        int n = homekit_srtp_protect(&ctx->srtp, pkt, RTP_HEADER_SIZE + plen, cap);
        if (n < 0) return -1;
        ctx->send(ctx->user, pkt, (size_t)n);
        return 0;
}

int homekit_rtp_h264_send(homekit_rtp_h264_t *ctx,
                          const uint8_t *au, size_t au_len, uint32_t timestamp) {
        if (!ctx || !au) return -1;

        // Maximum RTP payload that still fits the MTU after header + SRTP tag.
        size_t max_payload = ctx->mtu - RTP_HEADER_SIZE - HOMEKIT_SRTP_AUTH_TAG_SIZE;

        // Collect NAL units first so we can mark the last packet of the access unit.
        const uint8_t *p = au, *end = au + au_len;
        const uint8_t *nals[64]; size_t nlens[64]; int ncount = 0;
        const uint8_t *nal; size_t nlen;
        while ((p = next_nal(p, end, &nal, &nlen)) != NULL) {
                if (nlen == 0) continue;
                if (ncount < 64) { nals[ncount] = nal; nlens[ncount] = nlen; ncount++; }
        }
        if (ncount == 0) return 0;

        int sent = 0;
        for (int i = 0; i < ncount; i++) {
                int last_nal = (i == ncount - 1);
                nal = nals[i]; nlen = nlens[i];

                if (nlen <= max_payload) {
                        // Single NAL unit packet.
                        if (emit_packet(ctx, nal, nlen, timestamp, last_nal) < 0) return -1;
                        sent++;
                } else {
                        // FU-A fragmentation.
                        uint8_t nal_hdr = nal[0];
                        const uint8_t *data = nal + 1;
                        size_t remaining = nlen - 1;
                        size_t frag_max = max_payload - 2;   // 2 bytes FU indicator+header
                        int first = 1;
                        while (remaining > 0) {
                                size_t fsz = remaining > frag_max ? frag_max : remaining;
                                int is_last_frag = (fsz == remaining);
                                uint8_t fu_ind = (uint8_t)((nal_hdr & 0xE0) | FU_A_TYPE);
                                uint8_t fu_hdr = (uint8_t)((first ? 0x80 : 0) |
                                                           (is_last_frag ? 0x40 : 0) |
                                                           (nal_hdr & 0x1F));
                                // Build the FU-A packet (RTP header + FU indicator + FU header
                                // + fragment) directly in the scratch buffer, then SRTP in place.
                                uint8_t *pkt = ctx->buf;
                                pkt[0] = 0x80;
                                int marker = (last_nal && is_last_frag) ? 0x80 : 0;
                                pkt[1] = (uint8_t)(marker | (ctx->payload_type & 0x7f));
                                pkt[2] = (uint8_t)(ctx->seq >> 8);
                                pkt[3] = (uint8_t)(ctx->seq);
                                pkt[4] = (uint8_t)(timestamp >> 24); pkt[5] = (uint8_t)(timestamp >> 16);
                                pkt[6] = (uint8_t)(timestamp >> 8);  pkt[7] = (uint8_t)(timestamp);
                                pkt[8]  = (uint8_t)(ctx->ssrc >> 24); pkt[9]  = (uint8_t)(ctx->ssrc >> 16);
                                pkt[10] = (uint8_t)(ctx->ssrc >> 8);  pkt[11] = (uint8_t)(ctx->ssrc);
                                pkt[12] = fu_ind;
                                pkt[13] = fu_hdr;
                                memcpy(pkt + 14, data, fsz);
                                ctx->seq++;
                                int n = homekit_srtp_protect(&ctx->srtp, pkt, 14 + fsz, ctx->mtu);
                                if (n < 0) return -1;
                                ctx->send(ctx->user, pkt, (size_t)n);
                                sent++;

                                data += fsz; remaining -= fsz; first = 0;
                        }
                }
        }
        return sent;
}
