#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>

#define UDP_PAYLOAD_SIZE 1400

/*
 * Wire-format frame type identifiers.
 *
 * These are intentionally NOT the QNX camera_frametype_t enum values:
 * the Windows receiver is built without the QNX SDK headers, so it has
 * no way to know the real enum values and must not guess them. The QNX
 * sender maps camera_buffer_t->frametype to one of these constants
 * (see FrameUtils.c) before it goes on the wire; the receiver only
 * ever needs to know these constants.
 */
#define WIRE_FRAMETYPE_UNKNOWN  0
#define WIRE_FRAMETYPE_YCBYCR   1
#define WIRE_FRAMETYPE_CBYCRY   2
#define WIRE_FRAMETYPE_RGB8888  3
#define WIRE_FRAMETYPE_BGR8888  4
#define WIRE_FRAMETYPE_NV12     5

#if defined(_MSC_VER)
#pragma pack(push, 1)
#define PACKET_PACKED
#else
#define PACKET_PACKED __attribute__((packed))
#endif

/*
 * Sent as-is in front of every UDP datagram. frametype/width/height/stride
 * describe the frame currently being reassembled and are repeated in every
 * packet of that frame (cheap: fixed 16 bytes) so the receiver can decode a
 * frame without any separate control channel and without assuming a fixed
 * camera mode.
 */
typedef struct PACKET_PACKED
{
    uint32_t frameNumber;
    uint16_t packetNumber;
    uint16_t totalPackets;
    uint16_t payloadSize;

    uint32_t frametype;
    uint32_t width;
    uint32_t height;
    uint32_t stride;

    /* Only meaningful when frametype == WIRE_FRAMETYPE_NV12: the byte offset
     * from the start of the frame to the UV plane, and the UV plane's row
     * stride. QNX's camera_frame_nv12_t explicitly warns these must not be
     * assumed to equal stride*height / stride, so they are carried on the
     * wire rather than recomputed by the receiver. Zero for every other
     * format. */
    uint32_t uvOffset;
    uint32_t uvStride;
} PacketHeader;

#if defined(_MSC_VER)
#pragma pack(pop)
#endif

#undef PACKET_PACKED

#endif
