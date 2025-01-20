# RIPPLES

Ripples is a program written in C designed for high performant transaction
processing. Application demonstrates a software architecture that optimizes
processing via several ways:

* CPU affinity for transaction processing
* Vector based socket reads and writes (wherever possible)
* Vector based data processing.
* Non blocking transaction handling threads.
* Optimized sharing of resources between threads.

Optimizations minimize data (packet) traversal through the kernel, minimize
context switching, and minimize CPU cache misses.
Architecture also demonstrates how a resource (object) could be efficiently
shared amongst threads without the use of locks, and how to move blocking
operations to dedicated threads.

For detailed explanation of software architecture see the doxygen generated
documentation. Build section has information how to build the application
including the doxygen based documentation.

As a demonstration of capabilities ripples implements a basic authoritative DNS
server. DNS database is not implemented as that is beyond the scope of what
this project aims to accomplish. See Usage section for more details.

[Detailed Documentation](https://g5unit.github.io/ripples/)

## Usage

To start the application run the compiled binary named "ripples". The basic
application functionality is that it listens for DNS queries (requests) over
UDP and TCP transport protocols, and provides answers (responses). There are
numerous settings that could be used to fine tune software behavior to match
requirements and optimize behavior.

DNS response behavior is as such:

* All A query types are responded to with IP address of 127.0.0.1.
* All other query types are responded to with rcode NOTIMPL.
* Authority section in answers is populated with ns.example.com.
* Additional section is populated with ns.example.com A record of 127.0.0.1 and
AAAA record of ::1 .

For a list of CLI options application has run "ripples --help".

## Build

Default C compiler used is "clang". If you wish to use a different compiler such
as "gcc" edit the Makefile and change the CC variable from "clang" to "gcc".
C standard used is gnu17.

To compile the application run ```make ripples```. This will build the binary
which will be in build/bin directory.

The following make options are available:

    Usage: make <command>

    Commands:

    help           Prints his help message.

    all            Builds all targets: ripples, ripples_debug, test, and doc.

    ripples        Builds the ripples application. Binary file will be build/bin/ripples.

    ripples_debug  Builds the ripples application with debug output compiled in.
                   See documentation for details on debug output.
                   Binary file will be build/bin/ripples_debug.

    test           Builds and runs unit tests in "test_c" directory. Binary for
                   unit tests is test_c/ctest. You can execute it as is or run
                   "test_c/ctests -h" to see options it supports.
                   Unit tests require criterion library to be installed on
                   the system. On Ubuntu Linux you can do so via:
                   apt install libcriterion-dev

    doc            Build documentation using doxygen. You need doxygen installed
                   on the system. Documentation will be in build/docs/html
                   directory. Once built, open index.html file in a browser.

    lfds           Build liblfds only.

    clean          Cleans all builds.

    clean_ripples  Cleans ripples build.

    clean_debug    Removes ripples_debug binary.

    clean_test     Removes test/ctests binary.

    clean_doc      Removes built documentation.

## Reporting Issues

To report issues in github project space.

## License

Ripples is released under MIT license. There are three third party pieces of
software used that have their own license.

* [liblfds](https://www.liblfds.org) is library of lock free data structures with a
[free to use license](https://www.liblfds.org/mediawiki/index.php?title=r7.1.1:Introduction#License).
None of the code in liblfds was modified.

* Set of NS utilities in rip_ns_utils.h and rip_ns_utils.c files have the
license below (also present as comment at top of each file). Code was modified.

        Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
        Copyright (c) 1996,1999 by Internet Software Consortium.
    
        Permission to use, copy, modify, and distribute this software for any
        purpose with or without fee is hereby granted, provided that the above
        copyright notice and this permission notice appear in all copies.
    
        THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
        WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
        MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
        ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
        WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
        ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
        OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

* UTHASH is used to implement LRU cache. None of the UTHASH code was modified.

        Copyright (c) 2003-2022, Troy D. Hanson  https://troydhanson.github.io/uthash/
        All rights reserved.

        Redistribution and use in source and binary forms, with or without
        modification, are permitted provided that the following conditions are met:

        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

        THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
        IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
        TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
        PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
        OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
        EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
        PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
        PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
        LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
        NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
        SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
