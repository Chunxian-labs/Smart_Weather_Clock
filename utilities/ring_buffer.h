#ifndef __RING_BUFFER_H_
#define __RING_BUFFER_H_
#define RING_BUFFER_SIZE 128
#include <stdint.h>
#include <stdbool.h>
typedef struct
{
    uint8_t data[RING_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
}ring_buffer_t;
void ring_buffer_init(ring_buffer_t *rb);
bool ring_buffer_write(ring_buffer_t *rb,uint8_t data);
bool ring_buffer_read(ring_buffer_t *rb,uint8_t *data);
bool ring_buffer_is_empty(const ring_buffer_t *rb);
bool ring_buffer_is_full(const ring_buffer_t *rb);
void ring_buffer_clear(ring_buffer_t *rb);
#endif