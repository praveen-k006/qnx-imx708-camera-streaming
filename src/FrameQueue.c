#include "FrameQueue.h"

void FrameQueue_init(FrameQueue *q)
{
    q->head = 0;
    q->tail = 0;
    q->count = 0;

    q->pushed = 0;
    q->popped = 0;
    q->dropped = 0;
    q->maxCount = 0;

    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
}

int FrameQueue_push(FrameQueue *q, Frame frame)
{
    pthread_mutex_lock(&q->mutex);

    if (q->count == FRAME_QUEUE_SIZE)
    {
        q->dropped++;

        pthread_mutex_unlock(&q->mutex);
        return 0;
    }

    q->frames[q->tail] = frame;

    q->tail = (q->tail + 1) % FRAME_QUEUE_SIZE;

    q->count++;

    q->pushed++;

    if (q->count > q->maxCount)
    {
        q->maxCount = q->count;
    }

    pthread_cond_signal(&q->cond);

    pthread_mutex_unlock(&q->mutex);

    return 1;
}

int FrameQueue_pop(FrameQueue *q, Frame *frame)
{
    pthread_mutex_lock(&q->mutex);

    while (q->count == 0)
    {
        pthread_cond_wait(&q->cond, &q->mutex);
    }

    *frame = q->frames[q->head];

    q->head = (q->head + 1) % FRAME_QUEUE_SIZE;

    q->count--;

    q->popped++;

    pthread_mutex_unlock(&q->mutex);

    return 1;
}
