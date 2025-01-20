/**
 * @file resource_utils.c
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
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "resource.h"
#include "utils.h"

/** Function releases resource data of type raw file.
 * 
 * @param resource Resource this data applies to.
 * @param buf      Data to be released.
 */
void
resource_release_raw_file(resource_t *resource, void *buf)
{
    if (buf != NULL) {
        free(buf);
    }
}

/** Function checks for change and if changed loads a file into memory buffer
 * as raw data.
 * 
 * Change is checked for by stating the file for change time.
 * 
 * @param resource Resource to check
 * @param buf      Where to store pointer to loaded data, on change.
 * @param buf_len  Length of buf available to use.
 * @param err      Buffer where to store error string if error was encountered.
 * @param err_len  Length of err buffer available to use.
 * 
 * @return           1 - Resource changed and was successfully loaded into buf
 *                   0 - Resource has not changed.
 *                  -1 - There was an error either checking of loading the
 *                       resource file. Error message is populated.
 */
int
resource_check_load_raw_file(resource_t *resource, void **buf, size_t *buf_len,
                             char *err, size_t err_len)
{
    int         fd = -1;
    struct stat file_stat;
    int         ret = 0;
    char       *err_static;
    char        err_str[err_len];
    
    /* Open file. */
    fd = open(resource->filepath, O_RDONLY);
    if (fd < 0) {
        /* Error opening file! */
        err_static = strerror(errno);
        goto ERR_END;
    }

    /* Stat file to verify type, get last modified time and size. */
    if (fstat(fd, &file_stat) != 0) {
        /* error stating file. */
        close(fd);
        err_static = strerror(errno);
        goto ERR_END;
    }

    /* Verify it is not a directory. */
    if (!S_ISREG(file_stat.st_mode)) {
        /* not a regular file. */
        err_static = "not a regular file";
        close(fd);
        goto ERR_END;
    }

    /* Check if changed since last read and load. */
    if (resource->create_time.tv_sec != file_stat.st_ctim.tv_sec &&
        resource->create_time.tv_nsec != file_stat.st_ctim.tv_nsec) {
        /* resource changed. */
        size_t res_len = file_stat.st_size;
        resource->create_time = file_stat.st_ctim;
        ret = utl_readall(fd, res_len, buf, err_str, err_len);
        close(fd);
        if (ret == 0) {
            *buf_len = res_len;
            return 1;
        } else {
            err_static = err_str;
            goto ERR_END;
        }
    }
    return 0;

ERR_END:
    if (err != NULL && err_len != 0) {
        snprintf(err, err_len, "resource file %s error: %s",
                 resource->name, err_static);
    }
    return -1;
}