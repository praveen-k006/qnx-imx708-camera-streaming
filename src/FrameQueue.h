#ifndef FRAMEQUEUE_H
#define FRAMEQUEUE_H

#include <pthread.h>
#include "Frame.h"

#define FRAME_QUEUE_SIZE 10

typedef struct
{
    Frame frames[FRAME_QUEUE_SIZE];

    int head;
    int tail;
    int count;

    pthread_mutex_t mutex;
    pthread_cond_t cond;

    unsigned long pushed;
    unsigned long popped;
    unsigned long dropped;
    int maxCount;

} FrameQueue;

void FrameQueue_init(FrameQueue *q);
void FrameQueue_destroy(FrameQueue *q);

int FrameQueue_push(FrameQueue *q, Frame frame);
int FrameQueue_pop(FrameQueue *q, Frame *frame);

#endif
