#include "UdpSender.h"
#include "Packet.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

static int sock = -1;
static struct sockaddr_in destAddr;

static uint32_t frameNumber = 0;

/* Pacing target: measured link ceiling (iperf3, UDP) was ~95 Mbit/s with
 * 1.2% loss even right at that rate, over the currently available network
 * adapter (100 Mbit/s class). 70 Mbit/s leaves real margin below that.
 * Previously there was no pacing at all (usleep(0) is a no-op), so all
 * ~1300+ packets of a frame were fired back-to-back, instantly exceeding
 * any 100Mbit/s-class link and exhausting the local send buffer
 * (ENOBUFS) almost every packet.
 *
 * Raise this once a faster link (e.g. a Gigabit adapter) is in place --
 * it is the only knob that needs to change. */
#define UDP_TARGET_BITS_PER_SEC 70000000UL

/* Sleeping after every single packet (~1300+ usleep() calls/frame) measured
 * at only ~15 Mbit/s actual throughput against this 70 Mbit/s target: this
 * QNX target's usleep() has a real minimum delay well above the ~164us a
 * per-packet sleep would need, so per-call scheduling overhead dominated
 * the requested duration. Batching the sleep into ~10ms chunks keeps the
 * requested delay far above that floor, so it actually reflects the target
 * rate instead of being swamped by per-call overhead. */
#define UDP_PACE_INTERVAL_USEC 10000UL

static unsigned long bytesSincePace = 0;

static void paceSend(size_t bytesSent)
{
    unsigned long batchBytes =
        (UDP_TARGET_BITS_PER_SEC / 8UL) * UDP_PACE_INTERVAL_USEC / 1000000UL;

    bytesSincePace += (unsigned long)bytesSent;

    if (bytesSincePace >= batchBytes)
    {
        usleep(UDP_PACE_INTERVAL_USEC);
        bytesSincePace = 0;
    }
}

/* sendto() failures are reported as a once-per-second summary rather than
 * per-packet: during an ENOBUFS burst (thousands of packets/sec), printing
 * every failure to the console is slow enough on its own to make the
 * backlog worse, which was masking the real cause. */
static unsigned long sendErrorCount = 0;
static int lastSendErrno = 0;
static time_t lastSendErrorReport = 0;

int UdpSender_init(const char *ip, int port)
{
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return -1;
    }
    int buf = 8*1024*1024;
    setsockopt(sock,
    		SOL_SOCKET,
			SO_SNDBUF,
			&buf,
			sizeof(buf));
    memset(&destAddr,0,sizeof(destAddr));

    /* Increase UDP send buffer */


    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(port);
    destAddr.sin_addr.s_addr = inet_addr(ip);

    printf("UDP Sender initialized (%s:%d)\n", ip, port);

    return 0;
}

void UdpSender_send(const void *data, size_t size)
{
    if (sock < 0)
        return;

    int ret = sendto(sock,
                     data,
                     size,
                     0,
                     (struct sockaddr *)&destAddr,
                     sizeof(destAddr));

    if (ret < 0)
    {
        sendErrorCount++;
        lastSendErrno = errno;

        time_t now = time(NULL);
        if (now != lastSendErrorReport)
        {
            fprintf(stderr,
                    "UdpSender: sendto failing (%lu error%s in the last interval, "
                    "last errno=%d: %s)\n",
                    sendErrorCount,
                    (sendErrorCount == 1) ? "" : "s",
                    lastSendErrno,
                    strerror(lastSendErrno));
            sendErrorCount = 0;
            lastSendErrorReport = now;
        }
    }
}

void UdpSender_sendFrame(const Frame *frame)
{
    uint32_t currentFrame = frameNumber++;

    uint16_t totalPackets =
        (frame->size + UDP_PAYLOAD_SIZE - 1) / UDP_PAYLOAD_SIZE;

    unsigned char packet[sizeof(PacketHeader) + UDP_PAYLOAD_SIZE];
/*  used for testing Ethernet d rate
    printf("Sending frame %u (%u packets)\n",
           currentFrame,
           totalPackets);
*/
    for (uint16_t i = 0; i < totalPackets; i++)
    {
        PacketHeader header;

        header.frameNumber = currentFrame;
        header.packetNumber = i;
        header.totalPackets = totalPackets;

        header.frametype = frame->wireFormat;
        header.width = (uint32_t)frame->width;
        header.height = (uint32_t)frame->height;
        header.stride = (uint32_t)frame->stride;
        header.uvOffset = frame->uvOffset;
        header.uvStride = frame->uvStride;

        size_t offset = (size_t)i * UDP_PAYLOAD_SIZE;

        size_t remaining = frame->size - offset;

        header.payloadSize =
            (remaining > UDP_PAYLOAD_SIZE)
                ? UDP_PAYLOAD_SIZE
                : (uint16_t)remaining;

        memcpy(packet,
               &header,
               sizeof(PacketHeader));

        memcpy(packet + sizeof(PacketHeader),
               frame->data + offset,
               header.payloadSize);

        size_t packetSize = sizeof(PacketHeader) + header.payloadSize;

        UdpSender_send(packet, packetSize);
        paceSend(packetSize);
    }
}


void UdpSender_close(void)
{
    if (sock >= 0)
    {
        close(sock);
        sock = -1;
    }
}
