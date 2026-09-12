#ifndef UDPSENDER_H
#define UDPSENDER_H

#include <stddef.h>
#include "Frame.h"

int UdpSender_init(const char *ip, int port);

void UdpSender_send(const void *data, size_t size);

void UdpSender_sendFrame(const Frame *frame);

void UdpSender_close(void);

#endif
