/**
 * @file channel.c
 * @author Faruk Grozdanic
 * @copyright Released under MIT license.
 *
 * Copyright (c) 2025 Faruk Grozdanic
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <assert.h>

#include <liblfds711.h>

#include "channel.h"
#include "utils.h"

/** Create a bss channel message object.
 * 
 * @param id Message ID
 * @param op Message operation, see @ref channel_bss_ops_t.
 * @param p  Pointer to data this message applies to.
 * 
 * @return   On success returns pointer to newly created message. Caller
 *           is responsible for releasing the message via @ref
 *           channel_bss_msg_release once message is no longer needed.
 */
channel_bss_msg_t *
channel_bss_msg_create(uint64_t id, channel_bss_ops_t op, void *p)
{
    channel_bss_msg_t *msg = malloc(sizeof(channel_bss_msg_t));

    CHECK_MALLOC(msg);
    *msg = (channel_bss_msg_t) {
        .id = id,
        .op = op,
        .p = p,
    };
    return msg;
}

/** Release (free memory) a bss message object.
 * 
 * @param msg Message to release.
 */
void
channel_bss_msg_release(channel_bss_msg_t *msg)
{
    if (msg != NULL) {
        if (msg->p != NULL) {
            free(msg->p);
        }
        free(msg);
    }
}

/** Send a message on bss part of the channel.
 * 
 * This is used by support (bss) threads to send message to vectorloop thread.
 * 
 * @param ch  Channel to send message on.
 * @param msg Message to send.
 * 
 * 
 * @return    On success returns 1, otherwise returns 0. 0 could only be
 *            returned if message could not be enqueued due channel being full.
 */
int
channel_bss_send(channel_bss_t *ch, channel_bss_msg_t *msg)
{
    return lfds711_queue_bss_enqueue(&ch->bss.qbsss, NULL, (void *)msg);

}

/** Receive a message from support (bss) part of the channel.
 * 
 * This is used by vectorloop threads to receive message from support thread.
 * 
 * @param ch  Channel to receive message on.
 * @param msg Pointer where to place the received message.
 * 
 * 
 * @return    On success returns 1, otherwise returns 0. 0 could only be
 *            returned if there are no messages in the channel.
 */
int
channel_bss_recv(channel_bss_t *ch, channel_bss_msg_t **msg)
{
    return lfds711_queue_bss_dequeue(&ch->bss.qbsss, NULL, (void **)msg);
}

/** Send a message on vectorloop part of the channel.
 * 
 * This is used by vectorloop thread to send message to support (bss) thread.
 * 
 * @param ch  Channel to send message on.
 * @param msg Message to send.
 *
 * @return    On success returns 1, otherwise returns 0. 0 could only be
 *            returned if message could not be enqueued due channel being full.
 */
int
channel_bssvl_send(channel_bss_t *ch, channel_bss_msg_t *msg)
{
    return lfds711_queue_bss_enqueue(&ch->vl.qbsss, NULL, (void *)msg);
}

/** Receive a message from vectorloop part of the channel.
 * 
 * This is used by support (bss) thread to receive message from vectorloop thread.
 * 
 * @param ch  Channel to receive message on.
 * @param msg Pointer where to place the received message.
 *
 * @return    On success returns 1, otherwise returns 0. 0 could only be
 *            returned if there are no messages in the channel.
 */
int
channel_bssvl_recv(channel_bss_t *ch, channel_bss_msg_t **msg)
{
    return lfds711_queue_bss_dequeue(&ch->vl.qbsss, NULL, (void **)msg);
}

/** Assign a new channel message ID from given base.
 * 
 * Base is incremented and value returned.
 *
 * @param id_base 
 * @return uint64_t 
 */
uint64_t
channel_bss_assign_msg_id(uint64_t *id_base)
{
    *id_base += 1;
    return *id_base;
}

/** Create a new log channel message object.
 * 
 * @param log_msg_id Log message ID, see @ref app_log_msg_id_t.
 * @param log_msg    Log message string, can be NULL.
 * @param exit       Is this a fatal message, meaning should the application
 *                   exit once message is logged.
 * @return           On success returns pointer to newly created message. Caller
 *                   is responsible for releasing the message via @ref
 *                   channel_log_msg_release once message is no longer needed. 
 */
channel_log_msg_t *
channel_log_msg_create(uint32_t log_msg_id, char *log_msg, bool exit)
{
    channel_log_msg_t *msg = malloc(sizeof(channel_log_msg_t));

    CHECK_MALLOC(msg);
    *msg = (channel_log_msg_t) {
        .log_msg_id = log_msg_id,
        .log_msg = log_msg,
        .exit = exit,
    };
    return msg;
}

/** Release (free memory) a log channel message object.
 * 
 * @param msg Message object to release.
 */
void
channel_log_msg_release(channel_log_msg_t *msg)
{
    if (msg != NULL) {
        if (msg->log_msg != NULL) {
            free(msg->log_msg);
        }
        free(msg);
    }
}

/** Send a message on log channel.
 * 
 * This is used by threads threads to send message to log thread.
 * 
 * @param ch  Channel to send message on.
 * @param msg Message to send.
 * 
 * 
 * @return    On success returns 1, otherwise returns 0. 0 could only be
 *            returned if message could not be enqueued due channel being full.
 */
int
channel_log_send(channel_log_t *ch, channel_log_msg_t *msg)
{
    return lfds711_queue_bss_enqueue(&ch->qbsss, NULL, (void *)msg);

}

/** Receive a message from log channel.
 * 
 * This is used by log thread to receive message from other thread.
 * 
 * @param ch  Channel to receive message on.
 * @param msg Pointer where to place the received message.
 * 
 * 
 * @return    On success returns 1, otherwise returns 0. 0 could only be
 *            returned if there are no messages in the channel.
 */
int
channel_log_recv(channel_log_t *ch, channel_log_msg_t **msg)
{
    return lfds711_queue_bss_dequeue(&ch->qbsss, NULL, (void **)msg);
}
