/*
 * ringbuffer.c
 *
 *  Created on: Mar 28, 2024
 *      Author: ADMIN
 */

#include "ringbuffer.h"

uint8_t ring_buff_push(RingBuff *ring_buff, uint8_t data)
{
	uint16_t next;

    next = ring_buff->head + 1;  // next is where head will point to after this write.
    if (next >= ring_buff->maxlen)
        next = 0;

    if (next == ring_buff->tail)  // if the head + 1 == tail, circular buffer is full
        return -1;

    ring_buff->buffer[ring_buff->head] = data;  // Load data and then move
    ring_buff->head = next;             // head to next data offset.
    return 0;  // return success to indicate successful push.
}

uint8_t ring_buff_pop(RingBuff *ring_buff_pop/*, uint8_t *data*/)
{
    int next;
    uint8_t data;

    if (ring_buff_pop->head == ring_buff_pop->tail)  // if the head == tail, we don't have any data
        return -1;

    next = ring_buff_pop->tail + 1;  // next is where tail will point to after this read.
    if(next >= ring_buff_pop->maxlen)
        next = 0;

    data = ring_buff_pop->buffer[ring_buff_pop->tail];  // Read data and then move
    ring_buff_pop->tail = next;              // tail to next offset.
    return data;  // return success to indicate successful push.
}

uint16_t ring_buff_available(RingBuff *ring_buff)
{
	if((ring_buff->head) < (ring_buff->tail))
	{
		return ring_buff->maxlen - (ring_buff->tail - ring_buff->head);
	}
	return (ring_buff->head-ring_buff->tail);
}

void ring_buff_init(RingBuff *ring_buff, uint8_t *buff, uint16_t len)
{
	ring_buff->buffer = buff;
	ring_buff->head = 0;
	ring_buff->tail = 0;
	ring_buff->maxlen = len;
}
