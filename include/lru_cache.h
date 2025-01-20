/**
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
/** \defgroup lrucache LRU Cache
 *
 * @brief Least Recently Used (LRU) cache is implemented using UT HASH.
 *        Instead of implementing complex timer framework with callbacks, 
 *        LRU cache is used to check for various timeouts. LRU cache is walked
 *        backwards and entries checked if timeout occurred. For efficiency
 *        number of entries walked per vectorloop iteration is limited. Walk
 *        stops when either the limit is reached, or an entry encountered that
 *        has not timed out.
 *  @{
 */
#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <uthash/uthash.h>

/** Macro to add entry to TCP connection LRU cache. */
#define LRU_CACHE_ADD(head, add) \
    HASH_ADD(hh, head, cid, sizeof(uint64_t), add)

/** Macro to remove entry from TCP connection LRU cache. */
#define LRU_CACHE_DEL(head, del) \
    HASH_DELETE(hh, head, del)

#endif /* End of LRU_CACHE_H */

/** @}*/
