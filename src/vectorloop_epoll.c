/**
 * @file vectorloop_epoll.c
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
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/epoll.h>

#include "constants.h"
#include "utils.h"

/** Create an epoll file descriptor.
 * 
 * From linux man page on epoll_create():
 *     
 *     
 *     ERRORS
 *     EINVAL size is not positive.
 *
 *     EINVAL (epoll_create1()) Invalid value specified in flags.
 *
 *     EMFILE The   per-user   limit   on   the    number    of    epoll    instances    imposed    by
 *            /proc/sys/fs/epoll/max_user_instances  was  encountered.   See  epoll(7) for further de‐
 *            tails.
 *
 *     EMFILE The per-process limit on the number of open file descriptors has been reached.
 *
 *     ENFILE The system-wide limit on the total number of open files has been reached.
 *
 *     ENOMEM There was insufficient memory to create the kernel object.
 *
 * @return On success returns a file descriptor (a nonnegative integer).
 *         On error message if printed to stderr and assert called.
 */
int
vl_epoll_create(void)
{
    /* 
    */
    int ep_fd = epoll_create1(0);
    if (ep_fd < 0) {
        fprintf(stderr, "vl_epoll_create() err no %d, error message: %s\n",
                errno, strerror(errno));
        assert(0);
    }

    return ep_fd;
}

/** Issue epoll wait on vectorloop epoll instance and return number of events
 * epoll_wait() returned. epoll_wait is called with timeout of 0.
 * 
 * If there was an error in which case epoll_wait() returned -1, function aborts
 * (throws an exception).
 * 
 * @param ep_fd          Vectorloop operating under.
 * @param ep_events      Array of epoll events.
 * @param num_max_events Maximum number of events to report. There MUST be at
 *                       least this many entries in ep_events array.
 * 
 * @return   On success returns number of events returned by epoll. 
 *           On error, message if printed to stderr and assert called.
 */
uint32_t
vl_epoll_wait(int ep_fd, struct epoll_event *ep_events, int num_max_events)
{
    int event_count = 0;

    event_count = epoll_wait(ep_fd, ep_events, num_max_events, 0);
    if (event_count < 0) {
        fprintf(stderr, "vl_epoll_wait() err no %d, error message: %s\n",
                errno, strerror(errno));
        assert(0);
    }
    return event_count;
}

/** Register an open socket with epoll for Edge Triggered read events.
 * 
 * @note If socket could not be registered with epoll a message is printed to
 *       stderr and assert called.
 * 
 * @param ep_fd Epoll file descriptor to register with.
 * @param fd    Socket to register
 * @param id    Event ID associated with socket.
 */
void
vl_epoll_ctl_reg_for_read_et(int ep_fd, int fd, uint64_t id)
{
    struct epoll_event ev = {
        .events = EPOLLIN | EPOLLET,
        .data.u64 = id,
    };

    if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {       
        fprintf(stderr, "vl_epoll_ctl_reg_for_read_et() err no %d, error message: %s\n",
                errno, strerror(errno));
        assert(0);
    }
}

/** Register an open socket with epoll for Edge Triggered read and write events.
 * 
 * @note If socket could not be registered with epoll a message is printed to
 *       stderr and assert called.
 * 
 * @param ep_fd Epoll file descriptor to register with.
 * @param fd    Socket to register
 * @param id    Event ID associated with socket.
 */
void
vl_epoll_ctl_reg_for_readwrite_et(int ep_fd, int fd, uint64_t id)
{
    struct epoll_event ev = {
        .events = EPOLLIN | EPOLLOUT | EPOLLET,
        .data.u64 = id,
    };

    if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {       
        fprintf(stderr, "vl_epoll_ctl_reg_for_readwrite_et() err no %d, error message: %s\n",
                errno, strerror(errno));
        assert(0);
    }
}

/** Remove (delete) a socket from epoll.
 * 
 * @note If socket could not be removed from epoll a message is printed to
 *       stderr and assert called.
 * 
 * @param ep_fd Epoll file descriptor.
 * @param fd    Socket to remove
 */
void
vl_epoll_ctl_del(int ep_fd, int fd)
{
    if (epoll_ctl(ep_fd, EPOLL_CTL_DEL, fd, NULL) != 0) {
        fprintf(stderr, "vl_epoll_ctl_del() err no %d, error message: %s\n",
                errno, strerror(errno));
        assert(0);
    }
}
