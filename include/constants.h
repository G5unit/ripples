/**
 * @file constants.h
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
/** \defgroup constants Constants
 * 
 * @brief Various constants used throughout the code are defined here.
 *        By rule all static numerals should be extracted into a constant. This
 *        makes code and code changes tidier, especially when tuning code
 *        parameters to fit application use.
 *        The one exception to this rule is query parsing when advancing a
 *        buffer pointer during parsing of data. Even then, constants are used
 *        whenever appropriate. 
 *  @{
 */
#ifndef CONSTANTS_H
#define CONSTANTS_H

/* DEFAULT CONFIGURATION SETTINGS */
/** Default setting for udp_enable configuration parameter. */
#define CFG_DEFAULT_UDP_ENABLE true

/** Default setting for udp_listener_port configuration parameter. */
#define CFG_DEFAULT_UDP_LISTENER_PORT 53

/** Default setting for udp_conn_vector_len configuration parameter. */
#define CFG_DEFAULT_UDP_CONN_VECTOR_LEN 8

/** Default setting for udp_socket_recvbuff_size configuration parameter. */
#define CFG_DEFAULT_UDP_SOCK_RECVBUFF_SIZE 0xfffff

/** Default setting for udp_socket_sendbuff_size configuration parameter. */
#define CFG_DEFAULT_UDP_SOCK_SENDBUFF_SIZE 0xfffff

/** Default setting for tcp_enable configuration parameter. */
#define CFG_DEFAULT_TCP_ENABLE true

/** Default setting for tcp_listener_pending_conns_max configuration parameter. */
#define CFG_DEFAULT_TCP_LIST_PEND_CONNS_MAX 1024

/** Default setting for tcp_listener_port configuration parameter. */
#define CFG_DEFAULT_TCP_LISTENER_PORT 53

/** Default setting for tcp_conns_per_vl_max configuration parameter. */
#define CFG_DEFAULT_TCP_CONN_PER_VL_MAX 100000

/** Default setting for tcp_listener_max_accept_new_conn configuration parameter. */
#define CFG_DEFAULT_TCP_LIST_ACCEPT_NEW_CONNS_MAX 8

/** Default setting for tcp_conn_socket_recvbuff_size configuration parameter. */
#define CFG_DEFAULT_TCP_SOCK_RECVBUFF_SIZE 0x800

/** Default setting for tcp_conn_socket_sendbuff_size configuration parameter. */
#define CFG_DEFAULT_TCP_SOCK_SENDBUFF_SIZE 0x3000

/** Default setting for tcp_conn_simultaneous_queries_count configuration parameter. */
#define CFG_DEFAULT_TCP_SIM_QUERY_COUNT 3

/** Default setting for tcp_keepalive configuration parameter. */
#define CFG_DEFAULT_TCP_KEEPALIVE 10000

/** Default setting for tcp_query_recv_timeout configuration parameter. */
#define CFG_DEFAULT_TCP_QUERY_RECV_TIMEOUT 2000

/** Default setting for tcp_query_send_timeout configuration parameter. */
#define CFG_DEFAULT_TCP_QUERY_SEND_TIMEOUT 2000

/** Default setting for UDP epoll_num_events configuration parameter. */
#define CFG_DEFAULT_EPOLL_NUM_EVENTS_UDP 8

/** Default setting for TCP epoll_num_events configuration parameter. */
#define CFG_DEFAULT_EPOLL_NUM_EVENTS_TCP 8

/** Default setting for process_thread_count configuration parameter. */
#define CFG_DEFAULT_VL_THREAD_COUNT 1

/** Default setting for loop_slowdown_one configuration parameter. */
#define CFG_DEFAULT_VL_SLOWDOWN_ONE 1

/** Default setting for loop_slowdown_two configuration parameter. */
#define CFG_DEFAULT_VL_SLOWDOWN_TWO 50

/** Default setting for loop_slowdown_three configuration parameter. */
#define CFG_DEFAULT_VL_SLOWDOWN_THREE 100

/** Default setting for application_log_name configuration parameter. */
#define CFG_DEFAULT_APP_LOG_NAME "ripples.log"

/** Default setting for application_log_path configuration parameter. */
#define CFG_DEFAULT_APP_LOG_FILEPATH "."

/** Default setting for query_log_buffer_size configuration parameter. */
#define CFG_DEFAULT_QUERY_LOG_BUF_SIZE 6553500

/** Default setting for query_log_base_name configuration parameter. */
#define CFG_DEFAULT_QUERY_LOG_BASE_NAME "dns_query_log"

/** Default setting for query_log_path configuration parameter. */
#define CFG_DEFAULT_QUERY_LOG_PATH "logs"

/** Default setting for query_log_rotate_size configuration parameter. */
#define CFG_DEFAULT_QUERY_LOG_ROTATE_SIZE 50000000


/** Default setting for resource_1_name configuration parameter. */
#define CFG_DEFAULT_RESOURCE_1_NAME "Resource_1"

/** Default setting for resource_1_filepath configuration parameter. */
#define CFG_DEFAULT_RESOURCE_1_FILEPATH "resource1.txt"

/** Default setting for resource_1_update_freq configuration parameter. */
#define CFG_DEFAULT_RESOURCE_1_UPDATE_FREQ 5


/* MIN & MAX bound settings for CLI options*/
/** MIN bound for configuration setting "tcp_keepalive" */
#define TCP_KEEPALIVE_MIN 1000
/** MAX bound for configuration setting "tcp_keepalive" */
#define TCP_KEEPALIVE_MAX 600000

/** MIN bound for configuration setting "dns_query_request_max_len" */
#define DNS_QUERY_REQUEST_MAX_LEN_MIN 512
/** MAX bound for configuration setting "dns_query_request_max_len" */
#define DNS_QUERY_REQUEST_MAX_LEN_MAX 1024

/** MIN bound for configuration setting "dns_query_response_max_len" */
#define DNS_QUERY_RESPONSE_MAX_LEN_MIN 512
/** MAX bound for configuration setting "dns_query_response_max_len" */
#define DNS_QUERY_RESPONSE_MAX_LEN_MAX 0x10000

/** MIN bound for configuration setting "epoll_num_events" */
#define EPOLL_NUM_EVENTS_MIN 3
/** MAX bound for configuration setting "epoll_num_events" */
#define EPOLL_NUM_EVENTS_MAX 1024

/** MIN bound for configuration setting "process_thread_count" */
#define PROCESS_THREAD_COUNT_MIN 1
/** MAX bound for configuration setting "process_thread_count".
 * This limit is enforced by use of CPU_ADD() used to set threads CPU affinity.
 * For more information see Linux manual page for cpu_set (man -s3 cpu_set).
*/
#define PROCESS_THREAD_COUNT_MAX 1024

/** MIN bound for configuration settings "tcp_listener_port" and
 * "udp_listener_port".
*/
#define TCP_UDP_PORT_MIN 1
/** MAX bound for configuration settings "tcp_listener_port" and
 * "udp_listener_port".
*/
#define TCP_UDP_PORT_MAX 65535

/** MIN bound for configuration setting "tcp_listener_pending_conns_max" */
#define TCP_LIST_PENDING_CONNS_MAX_MIN 1
/** MAX bound for configuration setting "tcp_listener_pending_conns_max" */
#define TCP_LIST_PENDING_CONNS_MAX_MAX 0xffff

/** MIN bound for configuration setting "tcp_listener_max_accept_new_conn" */
#define TCP_LIST_MAX_ACCEPT_NEW_CONN_MIN 1
/** MAX bound for configuration setting "tcp_listener_max_accept_new_conn" */
#define TCP_LIST_MAX_ACCEPT_NEW_CONN_MAX 1024

/** MIN bound for configuration setting "tcp_conn_socket_recvbuff_size" */
#define TCP_CONN_SO_RECVBUFF_MIN 514
/** MAX bound for configuration setting "tcp_conn_socket_recvbuff_size" */
#define TCP_CONN_SO_RECVBUFF_MAX 0xffff

/** MIN bound for configuration setting "tcp_conn_socket_sendbuff_size" */
#define TCP_CONN_SO_SENDBUFF_MIN 514
/** MAX bound for configuration setting "tcp_conn_socket_sendbuff_size" */
#define TCP_CONN_SO_SENDBUFF_MAX 0xfffff

/** MIN bound for configuration setting "tcp_conn_simultaneous_queries_count" */
#define TCP_CONN_SIM_QUERY_COUNT_MIN 1
/** MAX bound for configuration setting "tcp_conn_simultaneous_queries_count" */
#define TCP_CONN_SIM_QUERY_COUNT_MAX 0xff

/** MIN bound for configuration setting "udp_conn_vector_len" */
#define UDP_CONN_VECTOR_LEN_MIN 1
/** MAX bound for configuration setting "udp_conn_vector_len" */
#define UDP_CONN_VECTOR_LEN_MAX 0xffff

/** MIN bound for configuration setting "udp_conn_socket_recvbuff_size" */
#define UDP_CONN_SO_RECVBUFF_MIN 518
/** MAX bound for configuration setting "udp_conn_socket_recvbuff_size" */
#define UDP_CONN_SO_RECVBUFF_MAX 0xffffff

/** MIN bound for configuration setting "udp_conn_socket_sendbuff_size" */
#define UDP_CONN_SO_SENDBUFF_MIN 512
/** MAX bound for configuration setting "udp_conn_socket_sendbuff_size" */
#define UDP_CONN_SO_SENDBUFF_MAX 0xffffff

/** MIN bound for configuration setting "loop_slowdown_one", "loop_slowdown_two"
 *  and "loop_slowdown_three".
*/
#define VL_SLOWDOWN_MIN 1
/** MAX bound for configuration setting "loop_slowdown_one", "loop_slowdown_two"
 *  and "loop_slowdown_three".
 */
#define VL_SLOWDOWN_MAX 10000

/** Size of buffer to store error messages in. Code generates error
 * messages that are logged in application log. This governs the size of buffer
 * that error message is first generated into. Note that error messages are
 * strings hence the maximum length of error string is ERR_MSG_LENGTH -1 to
 * account for C string terminator '\0'.
 */
#define ERR_MSG_LENGTH 1024

/** Maximum number of unique names that we allow to be compressed in a DNS
 * response. When packing names into DNS response an array is used to keep track
 * of names already packed in the response so if the same name is being packed
 * compression could be used. This setting sets the size of such array.
 * 
 * This does not limit number of unique names in response, just number of unique
 * names that are compressed in response.
 */
#define DNS_RESPONSE_COMPRESSED_NAMES_MAX 64

/** Length of UDP IP_PKTINFO control message. Control message is used to
 * get destination IP information on received packets.
 * 
 * Since UDP listeners bind to IPADDR_ANY we need to find out the destination IP
 * (local IP on server that received the packet) so we could use that information
 * to set the appropriate IP from information on response packet. This information
 * is received via IP socket option IP_PKTINFO control message.
 * 
 * This setting MUSt be large enough to accommodate both IPv4 and IPv6 packet
 * info.
 */
#define UDP_MSG_CONTROL_LEN 64

/** Number of resources that we load and update periodically.
 * Currently this is set to 1 as a demonstration of application capabilities.
 */
#define RESOURCE_COUNT 1

/** Minimum time that resource loop will sleep. Before waking up and performing
 * an action.
 * 
 * Resource loop will sleep until next action time which is least time until a
 * resource should be checked. If wait time is limited by this setting as
 * a precaution against having large delta times configured oer resources.
 */
#define RESOURCE_LOOP_TOP_DELTA_TIME -10

/** Number of elements in liblfds bonded single producer single consumer queue.
 * Per liblfds: number_elements must be a positive integer power of 2.
 * 
 * This should NOT be changed application uses BSS queues to process single
 * transaction at at time.
 * 
 * This constant is used by resource and query log channels
 */
#define CHANNEL_BSS_QUEUE_LEN 2

/** Size of liblfds bonded single producer single consumer queue used
 * for application log messages. We allow more than one message to be in a log
 * queue as application log messages are not treated as transactions (not a
 * request-response model). Instead these are fire and forget messages sent to
 * application log thread that reads the messages from channels and writes then
 * to application log file. As such we allow multiple messages to be queued up
 * in a log channel.
 */
#define CHANNEL_LOG_QUEUE_LEN 1024

/** Time in seconds to re-attempt opening application log file.  */
#define APP_LOG_OPEN_WAIT_TIME 5

/** Application thread loop sleep time. Unit is in nanoseconds. 
 * Maximum value is 999,999,999 which is "1 second - 1 nanosecond".
 * The value here is set to 1 millisecond.
 */
#define APP_LOG_LOOP_SLEEP_TIME 1000000

/** Maximum length of file realpath. Several configuration settings
 * are specified as path (directory) + name (filename). realpath() is then
 * called to combine the two into one string.
 */
#define FILE_REALPATH_MAX 4096

/** Maximum time resource loop will wait for all vectorloops to send notifications
 * that resource was updated (pointer flip). If a vectorloop thread takes more than
 * 10s to run a single loop it means that something is wrong and it is blocked.
 * There SHOULD be no blocking operations in vectorloop.
 * Unit is nanoseconds.
 * This is set to 1 seconds.
 */
#define VL_RESOURCE_NOTIFY_WAIT_TIME_MAX 1000000000

/** Time in microseconds query log loop slows down (sleeps) for if in a single
 * iteration no data was written to query log.
 */
#define QUERY_LOG_LOOP_SLOWDOWN 1000

/** Time in microseconds query log loop waits to get channel message response
 * from vectorloop thread.
 */
#define QUERY_LOG_LOOP_MSG_WAIT_TIME 10

/** Maximum character length of query log filename. This includes
 * base (directory), '/', and filename.
 */
#define QUERY_LOG_FILENAME_MAX_LEN 4096

/** If there was an error opening (creating) query log file, this is the time to
 * wait before retrying to open query log file.
 */
#define QUERY_LOG_FILE_OPEN_RETRY_TIME 1000000

/** Minimum space (in bytes) that must be available in query log buffer for a
 * query to be logged. This should be set to maximum expected length of a 
 * single query log entry.
 */
#define QUERY_LOG_BUF_MIN_SPACE 0xffff

#endif /* End of CONSTANTS_H */

/** @}*/
