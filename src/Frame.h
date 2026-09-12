#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>
#include <stdlib.h>
#include <camera/camera_api.h>

typedef struct
{
    uint8_t *data;

    size_t size;

    camera_frametype_t frametype;

    /* Wire-format identifier (WIRE_FRAMETYPE_*) and geometry, needed by
     * UdpSender to fill in PacketHeader so the receiver can decode the
     * frame regardless of which format the camera is actually running. */
    uint32_t wireFormat;
    uint32_t width;
    uint32_t height;
    uint32_t stride;

    /* Only meaningful for NV12 (see Packet.h); zero otherwise. */
    uint32_t uvOffset;
    uint32_t uvStride;

} Frame;

#endif
