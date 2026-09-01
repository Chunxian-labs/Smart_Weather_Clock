#include "ring_buffer.h"
#include <stdio.h>
void ring_buffer_init(ring_buffer_t *rb)
{
    if(rb == NULL)
    {
        return;
    }
    rb->head = 0;
    rb->tail = 0;
}
bool ring_buffer_is_empty(const ring_buffer_t *rb)
{
    return (rb->head == rb->tail);
}
bool ring_buffer_is_full(const ring_buffer_t *rb)
{
    return (rb->tail == (rb->head+1)%RING_BUFFER_SIZE);
}
bool ring_buffer_write(ring_buffer_t *rb,uint8_t data)
{
    if(rb == NULL)
    {
        return false;
    }
    if(ring_buffer_is_full(rb))
    {
        return false;
    }
    rb->data[rb->head] = data;
    rb->head++;
    if(rb->head == RING_BUFFER_SIZE)
    {
        rb->head = 0;
    }
    return true;
}
bool ring_buffer_read(ring_buffer_t *rb,uint8_t *data)
{
    if(rb == NULL || data == NULL)
    {
        return false;
    }
    if(ring_buffer_is_empty(rb))
    {
        return false;
    }
    *data = rb->data[rb->tail];
    rb->tail++;
    if(rb->tail == RING_BUFFER_SIZE)
    {
        rb->tail = 0;
    }
    return true;
}
void ring_buffer_clear(ring_buffer_t *rb)
{
    if(rb == NULL)
    {
        return;
    }
    rb->head = 0;
    rb->tail = 0;
}