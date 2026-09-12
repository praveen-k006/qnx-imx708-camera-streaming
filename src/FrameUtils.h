#ifndef FRAMEUTILS_H
#define FRAMEUTILS_H

#include "Frame.h"
#include <camera/camera_api.h>

Frame copyFrame(camera_buffer_t *buffer);
void freeFrame(Frame *frame);
const char *frametypeName(camera_frametype_t frametype);

#endif
