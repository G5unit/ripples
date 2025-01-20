/**
 * @file vectorloop_epoll.h
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
/** \defgroup vlepoll Vectorloop Epoll
 *
 * @brief These are function that wrapp epoll to provide functionality suited
 *        for application purpose. 
 *        Only Edge Triggered epoll events are used.
 *  @{
 */
#ifndef VECTORLOOP_EPOLL_H
#define VECTORLOOP_EPOLL_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/epoll.h>

int      vl_epoll_create(void);
uint32_t vl_epoll_wait(int ep_fd, struct epoll_event *ep_events, int num_max_events);
void     vl_epoll_ctl_reg_for_read_et(int ep_fd, int fd, uint64_t id);
void     vl_epoll_ctl_reg_for_readwrite_et(int ep_fd, int fd, uint64_t id);
void     vl_epoll_ctl_del(int ep_fd, int fd);

#endif /* End of VECTORLOOP_EPOLL_H */

/** @}*/

