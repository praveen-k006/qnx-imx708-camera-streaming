#include "FrameUtils.h"
#include "Packet.h"

#include <stdlib.h>
#include <string.h>

Frame copyFrame(camera_buffer_t *buffer)
{
    Frame frame;

    frame.data = NULL;
    frame.size = 0;
    frame.frametype = CAMERA_FRAMETYPE_UNSPECIFIED;
    frame.wireFormat = WIRE_FRAMETYPE_UNKNOWN;
    frame.width = 0;
    frame.height = 0;
    frame.stride = 0;
    frame.uvOffset = 0;
    frame.uvStride = 0;

    if (buffer == NULL || buffer->framebuf == NULL)
    {
        return frame;
    }

    frame.frametype = buffer->frametype;

    switch (buffer->frametype)
    {
        case CAMERA_FRAMETYPE_NV12:
            /* Default format of the installed rpi4_camera_module3.conf
             * (ISP-enabled) sensor config: confirmed by reading the real
             * config file and camera_defs.h on the target SDP install,
             * not assumed. uv_offset/uv_stride are carried as-is per the
             * header's explicit warning that they must not be derived from
             * stride*height. */
            frame.wireFormat = WIRE_FRAMETYPE_NV12;
            frame.width  = buffer->framedesc.nv12.width;
            frame.height = buffer->framedesc.nv12.height;
            frame.stride = buffer->framedesc.nv12.stride;
            frame.uvOffset = (uint32_t)buffer->framedesc.nv12.uv_offset;
            frame.uvStride = (uint32_t)buffer->framedesc.nv12.uv_stride;
            frame.size = (size_t)frame.uvOffset +
                         (size_t)frame.uvStride * (size_t)((frame.height + 1) / 2);
            break;

        case CAMERA_FRAMETYPE_YCBYCR:
            frame.wireFormat = WIRE_FRAMETYPE_YCBYCR;
            frame.width  = buffer->framedesc.ycbycr.width;
            frame.height = buffer->framedesc.ycbycr.height;
            frame.stride = buffer->framedesc.ycbycr.stride;
            break;

        case CAMERA_FRAMETYPE_CBYCRY:
            frame.wireFormat = WIRE_FRAMETYPE_CBYCRY;
            frame.width  = buffer->framedesc.cbycry.width;
            frame.height = buffer->framedesc.cbycry.height;
            frame.stride = buffer->framedesc.cbycry.stride;
            break;

        case CAMERA_FRAMETYPE_RGB8888:
            frame.wireFormat = WIRE_FRAMETYPE_RGB8888;
            frame.width  = buffer->framedesc.rgb8888.width;
            frame.height = buffer->framedesc.rgb8888.height;
            frame.stride = buffer->framedesc.rgb8888.stride;
            break;

        case CAMERA_FRAMETYPE_BGR8888:
            frame.wireFormat = WIRE_FRAMETYPE_BGR8888;
            frame.width  = buffer->framedesc.bgr8888.width;
            frame.height = buffer->framedesc.bgr8888.height;
            frame.stride = buffer->framedesc.bgr8888.stride;
            break;

        default:
            return frame;
    }

    if (buffer->frametype != CAMERA_FRAMETYPE_NV12)
    {
        frame.size = (size_t)frame.stride * (size_t)frame.height;
    }

    frame.data = malloc(frame.size);

    if (frame.data == NULL)
    {
        frame.size = 0;
        return frame;
    }

    memcpy(frame.data, buffer->framebuf, frame.size);

    return frame;
}

const char *frametypeName(camera_frametype_t frametype)
{
    switch (frametype)
    {
        case CAMERA_FRAMETYPE_NV12:    return "NV12";
        case CAMERA_FRAMETYPE_YCBYCR:  return "YCBYCR";
        case CAMERA_FRAMETYPE_CBYCRY:  return "CBYCRY";
        case CAMERA_FRAMETYPE_RGB8888: return "RGB8888";
        case CAMERA_FRAMETYPE_BGR8888: return "BGR8888";
        default:                       return "UNKNOWN";
    }
}

void freeFrame(Frame *frame)
{
    if (frame == NULL)
        return;

    if (frame->data != NULL)
    {
        free(frame->data);
        frame->data = NULL;
    }

    frame->size = 0;
}
