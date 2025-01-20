/**
 * @file channel.h
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
/** \defgroup channels Channels
 * 
 * @brief Channels are ways of exchanging data between threads. 
 *        libfsds (Lock Free Data Structures) are used to form channels and
 *        exchange transactions or uni-directional messages.
 * 
 *        Resource channel(s) are used to notify DNS query processing threads
 *        when a resource has changed. Messages are transactional in nature
 *        (request-response).
 * 
 *        Query log channel is used to notify DNS query aprocessing threads when
 *        to flip query log buffers.
 * 
 *        Log channel is used to send uni-directional log messages to application
 *        logging thread which in turn logs messages to file (disk).
 *  @{
 */
#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <liblfds711.h>

#include "constants.h"

/** Enumerated bss channel operation codes. */
typedef enum channel_bss_ops_e {
    /** Op code for resource 1. */
    CH_OP_RES_SET_RESOURCE1 = 0,

    /** Op code for resource 2. */
    CH_OP_RES_SET_RESOURCE2,

    /** Op code for query log flip. */
    CH_OP_QUERY_LOG_FLIP
} channel_bss_ops_t;

/** Structure describes an bss channel message. */
typedef struct channel_bss_msg_s {
    /** Message ID. */
    uint64_t id;

    /** Operation this message is for. */
    channel_bss_ops_t op;

    /** Pointer to data relating to specific operation.
     * For resource messages sender sets this to point to new data.
     * For query log flip messages responder sets this to buffer that has data
     * to be logged.
    */
    void *p;

    /** Result of the operation.
     * For resource messages sender (bss) sets this to 0, and
     * responder sets this to: 0 - error, 1 - success.
     * For query log flip messages responder sets this to length of data in
     * query log buffer.
     */
    size_t result;
} channel_bss_msg_t;

/** Structure describes an bss bonded channel queue. */
typedef struct channel_bss_queue_s {
    /** Channel queue. */
    struct lfds711_queue_bss_element qbsse[CHANNEL_BSS_QUEUE_LEN];

    /** Channel state*/
    struct lfds711_queue_bss_state qbsss;
} channel_bss_queue_t;

/** Structure describes an bss channel, Channel has two queues. Each queue is
 * unidirectional.
 */
typedef struct channel_bss_s {
    /** Admin queue, used by admin to send messages to vectorloop. 
     * Vectorloop reads messages from this queue.
     */
    channel_bss_queue_t bss;
 
    /** Vectorloop queue, used by vectorloop to send messages to bss.
     * Admin reads messages from this queue.
     */
    channel_bss_queue_t vl;
} channel_bss_t;


channel_bss_msg_t * channel_bss_msg_create(uint64_t id,
                                               channel_bss_ops_t op,
                                               void *p);
void     channel_bss_msg_release(channel_bss_msg_t *msg);
int      channel_bss_send(channel_bss_t *ch, channel_bss_msg_t *msg);
int      channel_bss_recv(channel_bss_t *ch, channel_bss_msg_t **msg);
int      channel_bssvl_send(channel_bss_t *ch, channel_bss_msg_t *msg);
int      channel_bssvl_recv(channel_bss_t *ch, channel_bss_msg_t **msg);
uint64_t channel_bss_assign_msg_id(uint64_t *id_base);


/** Structure describes a log channel message. */
typedef struct channel_log_msg_s {
    /** ID of one of the predefined log messages. If set (!= 0)
     * then the predefined message is logged. Otherwise string pointed to by
     * log_msg is logged.
    */
    uint32_t log_msg_id;

    /** Pointer to data that is to be logged. This must be a continuous buffer
     * which is freed once logged to disk. Can be NULL.
     */
    char *log_msg;

    /** Indicates if this is fatal error which indicates that once logged
     * program should exit.
     */
    bool exit;
} channel_log_msg_t;

/** Structure describes a log channel.  */
typedef struct channel_log_s {
   /** Channel queue. */
    struct lfds711_queue_bss_element qbsse[CHANNEL_LOG_QUEUE_LEN];

    /** Channel state*/
    struct lfds711_queue_bss_state qbsss;
} channel_log_t;

channel_log_msg_t * channel_log_msg_create(uint32_t log_msg_id, char *log_msg,
                                           bool exit);
void channel_log_msg_release(channel_log_msg_t *msg);
int  channel_log_send(channel_log_t *ch, channel_log_msg_t *msg);
int  channel_log_recv(channel_log_t *ch, channel_log_msg_t **msg);

#endif /* End of  CHANNEL_H */

/** @}*/
