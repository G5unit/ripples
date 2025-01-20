# Architecture

\tableofcontents

![Software Architecture Diagram](software_architecture.svg "Software Architecture Diagram")

It is known and has been proven that establishing CPU affinity when processing
network transactions achieves high system performance. To do so network data, or
datagram, should be processed on the same CPU by kernel and user space
application. By default network interface cards (NIC) use a single circular
buffer, also known as qring, where they store received (RX) packets. Most modern
NICs support configuring multiple RX buffers, one per CPU core, so processing
could be spread out across multiple CPU cores. NICs use a 4 or 5 point tuple
hash to steer packets for a TCP or UDP flow to the same RX buffer so a unique
flow is always processed by the same CPU. There are various flow steering
technologies such as RSS, RPS, RFS. For details on these see Linux networking
documentation.
Application can use multiple threads, each bound to a CPU core, to handle
datagram processing. This way network packets for a specific flow are always
handled by the same thread, reducing the need to keep an application wide
connection flow context. This also simplifies software code, as  same code that
runs on a single thread and processes network connection data could be invoked
to run on multiple number of threads.

All possible tasks should be offloaded to other threads, so the transaction processing
thread does not need to be unnecessarily blocked by doing non critical processing.
Amongst others, this includes resource management and logging.

## Datagram traversal through the application

The core concept of Ripples is a Vectorloop. This is a continuously running loop
with following steps done each loop iteration:

- check if there are messages from other threads it should handle (resource changes)
- read data from network sockets
- parse data into DNS queries
- resolve DNS queriers into answers
- pack answers (encode into format suitable for transmission)
- send DNS responses
- log DNS queries

Each step is done against a vector of data to optimize use of primed CPU cache.
This provides better performance than if these steps are done one query
at a time.

## Sharing resources amongst threads

Ripples application is an authoritative DNS server. DNS servers use zones
for data needed to respond to a query. Modern servers also use a GeoIP
location database to find out where the request originated from so they
could provide a response that is closest to the origination point.
DNS and GeoIP databases are just some of the resources a server would use.
These resources are usually in memory, could take large amounts of space (disk
or RAM), and also are subject to constant change. Application needs to
be able to absorb resource changes without impact on performance.
The efficient way to handle shared resources is to have a single thread, one that
does not process DNS queries, handle loading of resource into memory when resource
changes. Once an updated resource is loaded into its own memory space, each
thread is notified that it should switch from using the old resource data to
using the newly loaded resource data. This is usually just a pointer switch.
Once all threads process the pointer switch old data can be released (memory
freed). This approach works well for resources that are read only.

Ripples uses liblfds (lock free data structures) to create channels for
inter thread communication.

A dummy "resource_1" is present in ripples to demonstrate this approach.

## Offloading logging to dedicated threads: applciation log, query log

Threads that process queries should not have any blocking actions on them, such
as reading from or writing to disk. All such actions should be offloaded
to separate threads dedicated for the task.

Ripples offloads application logging to a dedicated thread via use of channels (liblfds).
Each thread sends a message to application logging thread with message that
should be logged. These are one way channels as application logging thread only receives
messages over channels.

For query logging Ripples uses dual buffers to log DNS queries, there is an active
and standby buffer. Queries are logged by vectorloop into active buffer. There is
a dedicated thread that writes data from these buffers to disk. This dedicated
thread notifies vectorloop thread that it should switch (flip) its active and standby buffers.
Once the flip is done the inactive (standby) buffer is accessed by logging thread
and data from the buffer written to disk. When data is written to disk the process
repeats and buffers are flipped once again.

## TCP timeouts

To avoid costly timer and callbacks implementation, ripples uses a Least Recently
Used (LRU) Cache. Each vectorloop iteration the cache is checked, starting with
the oldest entry in cache, if timeout was reached. Currently, the only timeouts
Ripples implements are one relating to TCP connections.

## Balancing TCP vs UDP request processing

TCP requests are received over TCP connections where each connection has its own
socket. UDP requests are received over a single socket. To balance the ratio
of TCP vs UDP queries processed each **vectorloop iteration**, the following exist:

- There are two epoll file descriptors, one for TCP, and one for UDP. Number of
events each epoll fd returns per epoll_wait() call is tunable.
- Number of UDP datagrams (packets) read in via a single call to recvmmsg() is tunable
- Number of new TCP connections accepted is tunable
- TCP is a stream of data and it is possible that a single read from socket
resulted in multiple queries read in. Number of simultaneous queries to process
per TCP connection is tunable.

For a full list of Ripples options see Usage.

## Metrics

Application metrics are stored in a single metrics object. It is fairly simple to
create a dedicated thread that at a cadence reports (writes to disk, or sends
over network) metrics and also resets the metric counters.
