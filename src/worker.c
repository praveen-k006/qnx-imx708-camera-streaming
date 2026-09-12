#include "worker.h"
#include "FrameQueue.h"
#include "FrameUtils.h"
#include "UdpSender.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

extern FrameQueue g_frameQueue;

void *workerThread(void *arg)
{
    (void)arg;

    Frame frame;

    unsigned long frameCount = 0;
    size_t lastFrameSize = 0;

    time_t lastTime = time(NULL);

    while (1)
    {
        if (FrameQueue_pop(&g_frameQueue, &frame))
        {
            frameCount++;
            lastFrameSize = frame.size;

            UdpSender_sendFrame(&frame);

            freeFrame(&frame);
        }

        time_t currentTime = time(NULL);

        if (currentTime != lastTime)
        {
            printf("\n");
            printf("============= QoS =============\n");
            printf("FPS              : %lu\n", frameCount);
            printf("Frame Size       : %zu bytes\n", lastFrameSize);
            printf("Frames Processed : %lu\n", frameCount);
            printf("================================\n");

            frameCount = 0;
            lastTime = currentTime;
        }
    }

    return NULL;
}
