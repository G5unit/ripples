/**
 * @file config.c
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
#include <getopt.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "constants.h"
#include "rip_ns_utils.h"
#include "utils.h"

/** Enumerated CLI option indexes. */
typedef enum cfg_opt_long_index_e {
    OPT_UNKNOWN = 0,
    OPT_HELP,

    OPT_UDP_ENABLE,
    OPT_UDP_LISTENER_PORT,
    OPT_UDP_SOCK_RECV_BUFF_SIZE,
    OPT_UDP_SOCK_SEND_BUFF_SIZE,
    OPT_UDP_CONN_VECTOR_LEN,

    OPT_TCP_ENABLE,
    OPT_TCP_LIST_PENDING_CONNS_MAX,
    OPT_TCP_LISTENER_PORT,
    OPT_TCP_CONN_PER_VL_MAX,
    OPT_TCP_LIST_MAX_ACCEPT_NEW_CONNS,
    OPT_TCP_CONN_SOCKET_RECV_BUFF_SIZE,
    OPT_TCP_CONN_SOCKET_SEND_BUFF_SIZE,
    OPT_TCP_CONN_SIMULTANEOUS_QUERY_COUNT,
    OPT_TCP_KEEPALIVE,
    OPT_TCP_QUERY_RECV_TIMEOUT,
    OPT_TCP_QUERY_SEND_TIMEOUT,

    OPT_EPOLL_NUM_EVENTS_TCP,
    OPT_EPOLL_NUM_EVENTS_UDP,
    OPT_PROCESS_THREAD_COUNT,
    OPT_PROCESS_THREAD_MASKS,

    OPT_LOOP_SLOWDOWN_ONE,
    OPT_LOOP_SLOWDOWN_TWO,
    OPT_LOOP_SLOWDOWN_THREE,

    OPT_APP_LOG_NAME,
    OPT_APP_LOG_PATH,

    OPT_QUERY_LOG_BUFFER_SIZE,
    OPT_QUERY_LOG_BASE_NAME,
    OPT_QUERY_LOG_PATH,
    OPT_QUERY_LOG_ROTATE_SIZE,

} cfg_opt_long_index_t;

/** Outputs a usage message to standard out. */
static void
usage(void)
{
    fprintf(stdout,"Usage:\n");

    fprintf(stdout,"\tripples [options]\n\n");

    fprintf(stdout,"Options are:\n\n");

    fprintf(stdout,"--help\n"
                   "\tPrint out this help message and exit.\n\n");

    fprintf(stdout,"--udp_enable (True|False)\n"
                   "\tEnable receiving DNS queries over UDP transport protocol.\n"
                   "\tDefault is True.\n\n");

    fprintf(stdout,"--udp_listener_port (number 1-65535)\n"
                   "\tSpecify port to receive DNS queries over UDP transport protocol.\n"
                   "\tDefault port is 53.\n\n");

    fprintf(stdout,"--udp_socket_recvbuff_size (number 518-16777215)\n"
                   "\tSet size of UDP socket receive buffer\n"
                   "\tDefault size is 1048575.\n\n"
                   "\tNOTE: Linux kernel setting net.core.rmem_max defines the upper cap of\n"
                   "\tthe maximum requestable buffer size. Importantly, this value must be\n"
                   "\tgreater than net.core.rmem_default.\n\n");

    fprintf(stdout,"--udp_socket_sendbuff_size (number 512-16777215)\n"
                   "\tSet size of UDP socket send buffer\n"
                   "\tDefault size is 1048575.\n\n"
                   "\tNOTE: Linux kernel setting net.core.rmem_max defines the upper cap of\n"
                   "\tthe maximum requestable buffer size. Importantly, this value must be\n"
                   "\tgreater than net.core.rmem_default.\n\n");    

    fprintf(stdout,"--udp_conn_vector_len (number 1-65535)\n"
                   "\tSpecify vector length for recvmmsg() call. This sets the maximum\n"
                   "\tnumber of UDP packets to read process per vectorloop\n"
                   "\tDefault is 8.\n\n");


    fprintf(stdout,"--tcp_enable (True|False)\n"
                   "\tEnable receiving DNS queries over TCP transport protocol.\n"
                   "\tDefault is True.\n\n");

    fprintf(stdout,"--tcp_listener_pending_conns_max (number 1-65535)\n"
                   "\tSpecify how many pending connections TCP listener will keep in queue\n"
                   "\twaiting to be accepted. WHen this number is reached any subsequent incoming\n"
                   "\tTCP connection request will be rejected. This setting is passed as argument\n"
                   "\tto system call listen().\n"
                   "\tDefault is 1024.\n\n");

    fprintf(stdout,"--tcp_listener_port (number 1-65535)\n"
                   "\tSpecify port to receive DNS queries over TCP transport protocol.\n"
                   "\tDefault port is 53.\n\n");

    fprintf(stdout,"--tcp_listener_max_accept_new_conn (number 1-65535)\n"
                   "\tSpecify how may new TCP connections should be accepted per Vectorloop\n"
                   "\titeration. Each Vectorloop iteration processes new queries received\n"
                   "\tover UDP and existing TCP connections. In addition, new TCP connections\n"
                   "\tare accepted. This parameter can be used to balance Vectorloop iteration\n"
                   "\ttime spent on each of these. Additional relevant options are\n"
                   "\t\"--tcp_conn_simultaneous_queries_count\" and \"--udp_conn_vector_len\".\n"
                   "\tDefault is 8.\n\n");

    fprintf(stdout,"--tcp_conn_socket_recvbuff_size (number 514-65535)\n"
                   "\tSet size of TCP socket receive buffer\n"
                   "\tDefault size is 2048.\n\n"
                   "\tNOTE: Linux kernel setting net.core.rmem_max defines the upper cap of\n"
                   "\tthe maximum requestable buffer size. Importantly, this value must be\n"
                   "\tgreater than net.core.rmem_default.\n\n");

    fprintf(stdout,"--tcp_conn_socket_sendbuff_size (number 512-65535)\n"
                   "\tSet size of UDP socket send buffer\n"
                   "\tDefault size is 12288.\n\n"
                   "\tNOTE: Linux kernel setting net.core.rmem_max defines the upper cap of\n"
                   "\tthe maximum requestable buffer size. Importantly, this value must be\n"
                   "\tgreater than net.core.rmem_default.\n\n");    

    fprintf(stdout,"--tcp_conn_simultaneous_queries_count (number 1-255)\n"
                   "\tA read from TCP socket could result in multiple queries read in. This\n"
                   "\tsetting limits number of queries that would be processed per read\n"
                   "\t(vectorloop iteration) from a TCP connection.\n"
                   "\tDefault is 3.\n\n"); 

    fprintf(stdout,"--tcp_keepalive (miliseconds 1000-600000)\n"
                   "\tTime that TCP will stay IDLE, waiting for new queries to arrive. This\n"
                   "\ttimer is started after a query is processed, it does not apply when\n"
                   "\ta connection is first established. When this timer is reached TCP connection\n"
                   "\tis closed.\n"
                   "\tDefault is 10000 (10 seconds).\n\n"); 

    fprintf(stdout,"--tcp_query_recv_timeout (miliseconds 1-UINTMAX_MAX)\n"
                   "\tTime to wait for query to be received. This timer applies when a\n"
                   "\tconnection is first established, as well as when a partial query\n"
                   "\tis received (waiting for bytes that completed the query request).\n"
                   "\tWhen this timer is reached TCP connection is closed.\n"
                   "\tDefault is 2000 (2 seconds).\n\n"); 

    fprintf(stdout,"--tcp_query_send_timeout (miliseconds 1-UINTMAX_MAX)\n"
                   "\tTime to wait for TCP socket to become writable so a query response\n"
                   "\tcould be sent. Timer starts from first write attempt for a query and\n"
                   "\tis reset when the full query is written to the TCP socket.\n"
                   "\tWhen this timer is reached TCP connection is closed.\n"
                   "\tDefault is 2000 (2 seconds).\n\n");

   fprintf(stdout,"--tcp_conns_per_vl_max (number 1-UINTMAX_MAX)\n"
                   "\tMaximum number of simultaneous TCP connections per vector loop\n"
                   "\tWhen this maximum is reached new TCP connections are rejected\n"
                   "\tDefault is 100000.\n\n"); 


    fprintf(stdout,"--epoll_num_events_tcp (number 3-1024\n"
                   "\tMaximum number of events to have reported in a single call to epoll.\n"
                   "\tThis settings includes TCP listeners and connections.\n"
                   "\tDefault is 8.\n\n");                  

    fprintf(stdout,"--epoll_num_events_udp (number 3-1024\n"
                   "\tMaximum number of events to have reported in a single call to epoll.\n"
                   "\tThis settings includes UDP listeners.\n"
                   "\tDefault is 8.\n\n");  

    fprintf(stdout,"--process_thread_count (number 1-1024)\n"
                   "\tSet number of query processing (vectorloop) theads to start. This does\n"
                   "\tot include application support threads such as logging threads nor\n"
                   "\tresource update thread. Number of support threads is fixed and not tunable.\n"
                   "\tDefault is 1.\n\n");

    fprintf(stdout,"--process_thread_masks (no mask)\n"
                   "\tBind each vectorloop thread to a specific CPU. Format of this option\n"
                   "\tvalue is a comma separated list of CPUs to bind each thread to. Index\n"
                   "\tin value corresponds to thread number.\n"
                   "\tI.E:\n"
                   "\t--process_thread_masks=1,2,3,4,8,9\n"
                   "\tsets CPU affinity for first six threads where thread 1 would bind\n"
                   "\tto CPU1, thread 2 would bind to CPU2, .. , thread 5 would bind\n"
                   "\tto CPU8, and thread 6 would bind to CPU9.\n"
                   "\tIf more bindings are provided than thread count (set by \n"
                   "\t--process_thread_count), the excess entries are ignored.\n"
                   "\tIf less bindings are provided than thread count, then the threads\n"
                   "\tfor which no CPU binding was provided would not have CPU affinity\n"
                   "\tset and those threads would be subject to dynamic CPU scheduling\n"
                   "\tunderlying operating system uses.\n"
                   "\tDefault is no CPU affinity binding.\n\n"
                   "\tNote that on Linux first CPU has ID of 0 (zero), where as this\n"
                   "\toption starts numbering at 1\n\n");

    fprintf(stdout,"--loop_slowdown_one (microseconds 1-10000)\n"
                   "\tVectorloop is a continuously running loop. If there are no queries to\n"
                   "\tprocess the loop would needlessly consume CPU cycles. To avoid unnecessary\n"
                   "\tconsumption of CPU cycles vectorloop implements a slowdown policy where if\n"
                   "\tno queries were processed in a cycle it sleeps for a specified time before\n"
                   "\tcontinuing execution. The sleep time increases with each \"idle\" loop. The \n"
                   "\tpolicy is to sleep \"loop_slowdown_one\" time for 8 cycles, followed by\n"
                   "\t\"loop_slowdown_two\" time for next 8 cycles, followed by \"loop_slowdown_three\"\n"
                   "\ttime until a query arrives. Policy is reset (start from the begining) each time\n"
                   "\tvectorloop processed a query.\n"
                   "\tDefault is 1.\n\n");

    fprintf(stdout,"--loop_slowdown_two (microseconds 1-10000)\n"
                   "\tSee description for option \"loop_slowdown_one\"\n"
                   "\tDefault is 100.\n\n");

    fprintf(stdout,"--loop_slowdown_three (microseconds 1-10000)\n"
                   "\tSee description for option \"loop_slowdown_one\"\n"
                   "\tDefault is 50.\n\n");
        
    fprintf(stdout,"--app_log_name (string)\n"
                   "\tName of application log file. See related optin \"app_log_path\".\n"
                   "\tMaximum length of application log path plus name is 4096 which includes\n"
                   "\t\"/\" between them.\n"
                   "\tDefault is \"ripples.log\".\n\n");

    fprintf(stdout,"--app_log_path (string)\n"
                   "\tDirectory where to create application log file.\n"
                   "\tDefault is \".\" (directory where application is started in).\n\n");

    fprintf(stdout,"--query_log_buffer_size (number 1-UINTMAX_MAX)\n"
                   "\tSize in bytes for query log buffer. Each vectorloop allocates two\n"
                   "\tbuffers of this size used to write query logs to.\n"
                   "\tDefault is 6553500.\n\n");

    fprintf(stdout,"--query_log_base_name (string)\n"
                   "\tBase name for query log file. actual file name used is base name with\n"
                   "\tappended timestamp in rfc3999nano format.\n"
                   "\tDefault is \"dns_query_log\".\n\n");

    fprintf(stdout,"--query_log_path (string)\n"
                   "\tDirectory where to store query log files.\n"
                   "\tDefault is \"logs\", relative to directory application is started from.\n\n");

    fprintf(stdout,"--query_log_rotate_size (number 1-UINTMAX_MAX)\n"
                   "\tSize in bytes when exceeded a new query log file is opened. Note that this\n"
                   "\tsize limit is checked after each write (one write per vectorloop buffer)\n"
                   "\thence the actual file size would always exceed it.\n"
                   "\tDefault is 50000000.\n\n");

    fprintf(stdout,"Example:\n"
                   "\tripples --udp_listener_port=9053 --tcp_enable=false\n\n");
}

/** Parse string to unsigned long and ensure it falls within min & max bounds.
 * 
 * Maximum value is limited to maximum positive number or type ssize_t.
 * 
 * @param name Name of parameter being parsed.
 * @param str  String to parse into unsigned long.
 * @param min  Minumum bound for parsed unsigned long.
 * @param max  Maximum bound for parsed unsigned long.
 * 
 * @return     Returns parsed unsigned long if it falls within min & max bounds.
 *             Otherwise an error message is printed to stderr, and -1 is
 *             returned.
 */
static ssize_t
config_parse_unsigned_long_opt(const char *name, const char *str, size_t min, size_t max)
{
    size_t tmp_ul = 0;
    if (str_to_unsigned_long(&tmp_ul, optarg) != 0) {
        fprintf(stderr,"Error parsing option \"%s\", '%s' is not a recognized "
                       "number\n", name, optarg);
        return -1;
    }
    if (tmp_ul < min || tmp_ul > max) {
        fprintf(stderr,"Error parsing option \"%s\", '%s' is not within allowed "
                       "bounds of %lu - %lu\n",
                        name, optarg, min, max);
        return -1;
    }
    return tmp_ul;
}

/** Initialize configuration object to defaults.
 * 
 * @param cfg Configuration object to initialize.
 */
void config_init(config_t *cfg)
{
    /* Initialize Ripples configuration with default values. */
    *cfg = (config_t) {
        .udp_enable                          = CFG_DEFAULT_UDP_ENABLE,
        .udp_listener_port                   = CFG_DEFAULT_UDP_LISTENER_PORT,
        .udp_socket_recvbuff_size            = CFG_DEFAULT_UDP_SOCK_RECVBUFF_SIZE,
        .udp_socket_sendbuff_size            = CFG_DEFAULT_UDP_SOCK_SENDBUFF_SIZE,
        .udp_conn_vector_len                 = CFG_DEFAULT_UDP_CONN_VECTOR_LEN,

        .tcp_enable                          = CFG_DEFAULT_TCP_ENABLE,
        .tcp_listener_pending_conns_max      = CFG_DEFAULT_TCP_LIST_PEND_CONNS_MAX,
        .tcp_listener_port                   = CFG_DEFAULT_TCP_LISTENER_PORT,
        .tcp_listener_max_accept_new_conn    = CFG_DEFAULT_TCP_LIST_ACCEPT_NEW_CONNS_MAX,
        .tcp_conn_socket_recvbuff_size       = CFG_DEFAULT_TCP_SOCK_RECVBUFF_SIZE,
        .tcp_conn_socket_sendbuff_size       = CFG_DEFAULT_TCP_SOCK_SENDBUFF_SIZE,
        .tcp_conn_simultaneous_queries_count = CFG_DEFAULT_TCP_SIM_QUERY_COUNT,
        .tcp_keepalive                       = CFG_DEFAULT_TCP_KEEPALIVE,
        .tcp_query_recv_timeout              = CFG_DEFAULT_TCP_QUERY_RECV_TIMEOUT,
        .tcp_query_send_timeout              = CFG_DEFAULT_TCP_QUERY_SEND_TIMEOUT,
        .tcp_conns_per_vl_max                = CFG_DEFAULT_TCP_CONN_PER_VL_MAX,
    
        .epoll_num_events_tcp                = CFG_DEFAULT_EPOLL_NUM_EVENTS_TCP,
        .epoll_num_events_udp                = CFG_DEFAULT_EPOLL_NUM_EVENTS_UDP,
        .process_thread_count                = CFG_DEFAULT_VL_THREAD_COUNT,
    
        .loop_slowdown_one                   = CFG_DEFAULT_VL_SLOWDOWN_ONE,
        .loop_slowdown_two                   = CFG_DEFAULT_VL_SLOWDOWN_TWO,
        .loop_slowdown_three                 = CFG_DEFAULT_VL_SLOWDOWN_THREE,
    
        .resource_1_name                     = strdup(CFG_DEFAULT_RESOURCE_1_NAME),
        .resource_1_filepath                 = strdup(CFG_DEFAULT_RESOURCE_1_FILEPATH),
        .resource_1_update_freq              = CFG_DEFAULT_RESOURCE_1_UPDATE_FREQ,

        .application_log_name                = strdup(CFG_DEFAULT_APP_LOG_NAME),
        .application_log_path                = strdup(CFG_DEFAULT_APP_LOG_FILEPATH),

        .query_log_buffer_size               = CFG_DEFAULT_QUERY_LOG_BUF_SIZE,
        .query_log_base_name                 = strdup(CFG_DEFAULT_QUERY_LOG_BASE_NAME),
        .query_log_path                      = strdup(CFG_DEFAULT_QUERY_LOG_PATH),
        .query_log_rotate_size               = CFG_DEFAULT_QUERY_LOG_ROTATE_SIZE,

    };

    cfg->process_thread_masks = malloc(sizeof(size_t) * cfg->process_thread_count);
    CHECK_MALLOC(cfg->process_thread_masks);
    cfg->process_thread_masks[0] = 0;

    cfg->tcp_readbuff_size = cfg->tcp_conn_simultaneous_queries_count * 
                             (2 + RIP_NS_PACKETSZ);
    cfg->tcp_writebuff_size = cfg->tcp_conn_simultaneous_queries_count * 
                             (2 + RIP_NS_PACKETSZ);
}

/** Parse command line options into configuration object.
 * 
 * @param cfg  Configuration object where to store parsed options.
 * @param argc Array of options strings.
 * @param argv Number of options strings (entries in array).
 *
 * @return     On success returns 0, otherwise error occurred.
 */
int
config_parse_opts(config_t *cfg, int argc, char *argv[])
{
    int     c;
    ssize_t tmp_ul                           = 0;
    char    opt_process_thread_masks[0x1400] = {'\0'};
    int     entry_visited[512]               = {0};

    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            /* name, has_arg, flag, val */
            {"help",                                no_argument,       NULL, OPT_HELP},

            {"udp_enable",                          required_argument, NULL, OPT_UDP_ENABLE},
            {"udp_listener_port",                   required_argument, NULL, OPT_UDP_LISTENER_PORT},
            {"udp_socket_recvbuff_size",            required_argument, NULL, OPT_UDP_SOCK_RECV_BUFF_SIZE},
            {"udp_socket_sendbuff_size",            required_argument, NULL, OPT_UDP_SOCK_SEND_BUFF_SIZE},
            {"udp_conn_vector_len",                 required_argument, NULL, OPT_UDP_CONN_VECTOR_LEN},
            
            {"tcp_enable",                          required_argument, NULL, OPT_TCP_ENABLE},
            {"tcp_listener_pending_conns_max",      required_argument, NULL, OPT_TCP_LIST_PENDING_CONNS_MAX},
            {"tcp_listener_port",                   required_argument, NULL, OPT_TCP_LISTENER_PORT},
            {"tcp_conns_per_vl_max",                required_argument, NULL, OPT_TCP_CONN_PER_VL_MAX},
            {"tcp_listener_max_accept_new_conn",    required_argument, NULL, OPT_TCP_LIST_MAX_ACCEPT_NEW_CONNS},
            {"tcp_conn_socket_recvbuff_size",       required_argument, NULL, OPT_TCP_CONN_SOCKET_RECV_BUFF_SIZE},
            {"tcp_conn_socket_sendbuff_size",       required_argument, NULL, OPT_TCP_CONN_SOCKET_SEND_BUFF_SIZE},
            {"tcp_conn_simultaneous_queries_count", required_argument, NULL, OPT_TCP_CONN_SIMULTANEOUS_QUERY_COUNT},
            {"tcp_keepalive",                       required_argument, NULL, OPT_TCP_KEEPALIVE},
            {"tcp_query_recv_timeout",              required_argument, NULL, OPT_TCP_QUERY_RECV_TIMEOUT},
            {"tcp_query_send_timeout",              required_argument, NULL, OPT_TCP_QUERY_SEND_TIMEOUT},

            {"epoll_num_events_tcp",                required_argument, NULL, OPT_EPOLL_NUM_EVENTS_TCP},
            {"epoll_num_events_udp",                required_argument, NULL, OPT_EPOLL_NUM_EVENTS_UDP},
            {"process_thread_count",                required_argument, NULL, OPT_PROCESS_THREAD_COUNT},
            {"process_thread_masks",                required_argument, NULL, OPT_PROCESS_THREAD_MASKS},
            
            {"loop_slowdown_one",                   required_argument, NULL, OPT_LOOP_SLOWDOWN_ONE},
            {"loop_slowdown_two",                   required_argument, NULL, OPT_LOOP_SLOWDOWN_TWO},
            {"loop_slowdown_three",                 required_argument, NULL, OPT_LOOP_SLOWDOWN_THREE},


            {"app_log_name",                        required_argument, NULL, OPT_APP_LOG_NAME},
            {"app_log_path",                        required_argument, NULL, OPT_APP_LOG_PATH},

            {"query_log_buffer_size",               required_argument, NULL, OPT_QUERY_LOG_BUFFER_SIZE},
            {"query_log_base_name",                 required_argument, NULL, OPT_QUERY_LOG_BASE_NAME},
            {"query_log_path",                      required_argument, NULL, OPT_QUERY_LOG_PATH},
            {"query_log_rotate_size",               required_argument, NULL, OPT_QUERY_LOG_ROTATE_SIZE},
        
            {0, 0, 0,0} /* last entry MUST be all zeros per getopt_long() API. */
        };

        c = getopt_long(argc, argv, "",
                        long_options, &option_index);
        if (c == -1) {
            break;
        }

        if (entry_visited[c] > 0) {
            fprintf(stderr, "Error parsing options, duplicate option \"--%s\" detected.\n",
                    long_options[option_index].name);
            return  -1;
        }
        entry_visited[c] = 1;
  
        switch (c) {
        case OPT_HELP:
            /* help */
            usage();
            return -1;
            break;

        case OPT_UDP_ENABLE:
            /* udp_enable */
            if (str_to_bool(&cfg->udp_enable, optarg) != 0) {
                fprintf(stderr,"Error parsing option \"udp_enable\","
                               "'%s' is not a recognized argument (True|False)\n",
                               optarg);
                return -1;
            }
            break;

        case OPT_UDP_LISTENER_PORT:
            /* udp_listener_port */
            tmp_ul = config_parse_unsigned_long_opt(long_options[option_index].name,
                         optarg, 
                         TCP_UDP_PORT_MIN,
                         TCP_UDP_PORT_MAX);
            if (tmp_ul < 0) {
                return -1;
            }
            cfg->udp_listener_port = tmp_ul;
            break;

        case OPT_UDP_SOCK_RECV_BUFF_SIZE:
            /* udp_socket_recvbuff_size */
            tmp_ul = config_parse_unsigned_long_opt(long_options[option_index].name,
                         optarg, 
                         UDP_CONN_SO_RECVBUFF_MIN,
                         UDP_CONN_SO_RECVBUFF_MAX);
            if (tmp_ul < 0) {
                return -1;
            }
            cfg->udp_socket_recvbuff_size = tmp_ul;
            break;

        case OPT_UDP_SOCK_SEND_BUFF_SIZE:
            /* udp_socket_sendbuff_size */
            tmp_ul = config_parse_unsigned_long_opt(long_options[option_index].name,
                         optarg, 
                         UDP_CONN_SO_SENDBUFF_MIN,
                         UDP_CONN_SO_SENDBUFF_MAX);
            if (tmp_ul < 0) {
                return -1;
            }
            cfg->udp_socket_sendbuff_size = tmp_ul;
            break;

        case OPT_UDP_CONN_VECTOR_LEN:
            /* udp_conn_vector_len */
            tmp_ul = config_parse_unsigned_long_opt(long_options[option_index].name,
                         optarg, 
                         UDP_CONN_VECTOR_LEN_MIN,
                         UDP_CONN_VECTOR_LEN_MAX);
            if (tmp_ul < 0) {
                return -1;
            }
            cfg->udp_conn_vector_len = tmp_ul;
            break;

        case OPT_TCP_ENABLE:
            /* tcp_enable */
            if (str_to_bool(&cfg->tcp_enable, optarg) != 0) {
                fprintf(stderr,"Error parsing option \"tcp_enable\","
                               "'%s' is not a recognized argument (True|False)\n",
                               optarg);
                return -1;
            }
            break;

        case OPT_TCP_LIST_PENDING_CONNS_MAX:
            /* tcp_listener_pending_conns_max */
            tmp_ul = config_parse_unsigned_long_opt(long_options[option_index].name,
                         optarg, 
                         TCP_LIST_PENDING_CONNS_MAX_MIN,
                         TCP_LIST_PENDING_CONNS_MAX_MAX);
            if (tmp_ul < 0) {
                return -1;
            }
            cfg->tcp_listener_pending_conns_max = tmp_ul;
            break;

        case OPT_TCP_LISTENER_PORT:
            /* tcp_listener_port */
            tmp_ul = config_parse_unsigned_long_opt(long_options[option_index].name,
                         optarg, 
                         TCP_UDP_PORT_MIN,
                         TCP_UDP_PORT_MAX);
            if (tmp_ul < 0) {
                return -1;
            }
            cfg->tcp_listener_port = tmp_ul;
            break;

        case OPT_TCP_LIST_MAX_ACCEPT_NEW_CONNS:
            /* tcp_listener_max_accept_new_conn */
            tmp_ul = config_parse_unsigned_long_opt(long_options[option_index].name,
                         optarg, 
                         TCP_LIST_MAX_ACCEPT_NEW_CONN_MIN,
                         TCP_LIST_MAX_ACCEPT_NEW_CONN_MAX);
            if (tmp_ul < 0) {
                return -1;
            }
            cfg->tcp_listener_max_accept_new_conn = tmp_ul;
            break;

        case OPT_TCP_CONN_PER_VL_MAX:
            /* tcp_conns_per_vl_max */
            tmp_ul = config_parse_unsigned_long_opt(long_options[option_index].name,
                         optarg, 
                         1,
                         UINTMAX_MAX);
            if (tmp_ul < 0) {
                return -1;
            }
            cfg->tcp_listener_max_accept_new_conn = tmp_ul;
            break;

        case OPT_TCP_CONN_SOCKET_RECV_BUFF_SIZE:
            /* tcp_conn_socket_recvbuff_size */
            tmp_ul = config_parse_unsigned_long_opt(long_options[option_index].name,
                         optarg, 
                         TCP_CONN_SO_RECVBUFF_MIN,
                         TCP_CONN_SO_RECVBUFF_MAX);
            if (tmp_ul < 0) {
                return -1;
            }
            cfg->tcp_conns_per_vl_max = tmp_ul;
            break;

        case OPT_TCP_CONN_SOCKET_SEND_BUFF_SIZE:
            /* tcp_conn_socket_sendbuff_size */
            tmp_ul = config_parse_unsigned_long_opt(long_options[option_index].name,
                         optarg, 
                         TCP_CONN_SO_SENDBUFF_MIN,
                         TCP_CONN_SO_SENDBUFF_MAX);
            if (tmp_ul < 0) {
                return -1;
            }
            cfg->tcp_conn_socket_sendbuff_size = tmp_ul;
            break;

        case OPT_TCP_CONN_SIMULTANEOUS_QUERY_COUNT:
            /* tcp_conn_simultaneous_queries_count */
            tmp_ul = config_parse_unsigned_long_opt(long_options[option_index].name,
                         optarg, 
                         TCP_CONN_SIM_QUERY_COUNT_MIN,
                         TCP_CONN_SIM_QUERY_COUNT_MAX);
            if (tmp_ul < 0) {
                return -1;
            }
            cfg->tcp_conn_simultaneous_queries_count = tmp_ul;
            break;

        case OPT_TCP_KEEPALIVE:
            /* tcp_keepalive */
            tmp_ul = config_parse_unsigned_long_opt(long_options[option_index].name,
                         optarg, 
                         TCP_KEEPALIVE_MIN,
                         TCP_KEEPALIVE_MAX);
            if (tmp_ul < 0) {
                return -1;
            }
            cfg->tcp_keepalive = tmp_ul;
            break;

        case OPT_TCP_QUERY_RECV_TIMEOUT:
            /* tcp_query_recv_timeout */
            tmp_ul = config_parse_unsigned_long_opt(long_options[option_index].name,
                         optarg, 
                         1,
                         UINTMAX_MAX);
            if (tmp_ul < 0) {
                return -1;
            }
            cfg->tcp_query_recv_timeout = tmp_ul;
            break;

        case OPT_TCP_QUERY_SEND_TIMEOUT:
            /* tcp_query_send_timeout */
            tmp_ul = config_parse_unsigned_long_opt(long_options[option_index].name,
                         optarg, 
                         1,
                         UINTMAX_MAX);
            if (tmp_ul < 0) {
                return -1;
            }
            cfg->tcp_query_send_timeout = tmp_ul;
            break;

        case OPT_EPOLL_NUM_EVENTS_TCP:
            /* epoll_num_events_tcp */
            tmp_ul = config_parse_unsigned_long_opt(long_options[option_index].name,
                         optarg, 
                         EPOLL_NUM_EVENTS_MIN,
                         EPOLL_NUM_EVENTS_MAX);
            if (tmp_ul < 0) {
                return -1;
            }
            cfg->epoll_num_events_tcp = tmp_ul;
            break;

                case OPT_EPOLL_NUM_EVENTS_UDP:
            /* epoll_num_events_udp */
            tmp_ul = config_parse_unsigned_long_opt(long_options[option_index].name,
                         optarg, 
                         EPOLL_NUM_EVENTS_MIN,
                         EPOLL_NUM_EVENTS_MAX);
            if (tmp_ul < 0) {
                return -1;
            }
            cfg->epoll_num_events_udp = tmp_ul;
            break;
        
        case OPT_PROCESS_THREAD_COUNT:
            /* process_thread_count */
            tmp_ul = config_parse_unsigned_long_opt(long_options[option_index].name,
                         optarg, 
                         PROCESS_THREAD_COUNT_MIN,
                         PROCESS_THREAD_COUNT_MAX);
            if (tmp_ul < 0) {
                return -1;
            }

            /* reallocate cfg->process_thread_masks if it increased. */
            if (cfg->process_thread_count > tmp_ul) {
                cfg->process_thread_masks = realloc(cfg->process_thread_masks,
                                                    sizeof(size_t) * tmp_ul);
                CHECK_MALLOC(cfg->process_thread_masks);
                for (size_t i=cfg->process_thread_count; i < tmp_ul; i++) {
                    cfg->process_thread_masks[i] = 0;
                }
            }
            cfg->process_thread_count = tmp_ul;
            break;

        case OPT_PROCESS_THREAD_MASKS:
            /* process_thread_masks
             * Processing this option depends on having option "process_thread_count"
             * being set. We store the option string and process it last once
             * all the other options are parsed.
             */
            memcpy(&opt_process_thread_masks, optarg, 0x13FF);
            break;



        case OPT_LOOP_SLOWDOWN_ONE:
            /* loop_slowdown_one */
            tmp_ul = config_parse_unsigned_long_opt(long_options[option_index].name,
                         optarg, 
                         VL_SLOWDOWN_MIN,
                         VL_SLOWDOWN_MAX);
            if (tmp_ul < 0) {
                return -1;
            }
            cfg->loop_slowdown_one = tmp_ul;
            break;

        case OPT_LOOP_SLOWDOWN_TWO:
            /* loop_slowdown_two */
            tmp_ul = config_parse_unsigned_long_opt(long_options[option_index].name,
                         optarg, 
                         VL_SLOWDOWN_MIN,
                         VL_SLOWDOWN_MAX);
            if (tmp_ul < 0) {
                return -1;
            }
            cfg->loop_slowdown_two = tmp_ul;
            break;

        case OPT_LOOP_SLOWDOWN_THREE:
            /* loop_slowdown_three */
            tmp_ul = config_parse_unsigned_long_opt(long_options[option_index].name,
                         optarg, 
                         VL_SLOWDOWN_MIN,
                         VL_SLOWDOWN_MAX);
            if (tmp_ul < 0) {
                return -1;
            }
            cfg->loop_slowdown_three = tmp_ul;
            break;

        case OPT_APP_LOG_NAME:
            /* app_log_name */
            if (strlen(optarg) > FILE_REALPATH_MAX) {
                fprintf(stderr,"Error parsing option \"app_log_name\","
                               "'%s' length is greater than %d\n",
                               optarg, FILE_REALPATH_MAX);
                return -1;
            }
            cfg->application_log_name = strdup(optarg);
            if (cfg->application_log_name == NULL) {
                fprintf(stderr,"Error allocating string for option \"app_log_name\"\n");
                return -1;
            }
            break;

        case OPT_APP_LOG_PATH:
            /* app_log_path */
            if (strlen(optarg) > FILE_REALPATH_MAX) {
                fprintf(stderr,"Error parsing option \"app_log_path\","
                               "'%s' length is greater than %d\n",
                               optarg, FILE_REALPATH_MAX);
                return -1;
            }
            cfg->application_log_path = strdup(optarg);
            if (cfg->application_log_path == NULL) {
                fprintf(stderr,"Error allocating string for option \"app_log_path\"\n");
                return -1;
            }
            break;

        case OPT_QUERY_LOG_BUFFER_SIZE:
            /* query_log_buffer_size */
            tmp_ul = config_parse_unsigned_long_opt(long_options[option_index].name,
                         optarg, 
                         1,
                         UINTMAX_MAX);
            if (tmp_ul < 0) {
                return -1;
            }
            cfg->query_log_buffer_size = tmp_ul;
            break;

        case OPT_QUERY_LOG_BASE_NAME:
            /* query_log_base_name */
            if (strlen(optarg) > FILE_REALPATH_MAX) {
                fprintf(stderr,"Error parsing option \"query_log_base_name\","
                               "'%s' length is greater than %d\n",
                               optarg, FILE_REALPATH_MAX);
                return -1;
            }
            cfg->query_log_base_name = strdup(optarg);
            if (cfg->query_log_base_name == NULL) {
                fprintf(stderr,"Error allocating string for option \"query_log_base_name\"\n");
                return -1;
            }
            break;

        case OPT_QUERY_LOG_PATH:
            /* query_log_path */
            if (strlen(optarg) > FILE_REALPATH_MAX) {
                fprintf(stderr,"Error parsing option \"query_log_path\","
                               "'%s' length is greater than %d\n",
                               optarg, FILE_REALPATH_MAX);
                return -1;
            }
            cfg->query_log_path = strdup(optarg);
            if (cfg->query_log_path == NULL) {
                fprintf(stderr,"Error allocating string for option \"query_log_path\"\n");
                return -1;
            }
            break;

        case OPT_QUERY_LOG_ROTATE_SIZE:
            /* query_log_rotate_size */
            tmp_ul = config_parse_unsigned_long_opt(long_options[option_index].name,
                         optarg, 
                         1,
                         UINTMAX_MAX);
            if (tmp_ul < 0) {
                return -1;
            }
            cfg->query_log_rotate_size = tmp_ul;
            break;
        
        default:
            printf("Unrecognized option: %s\n", argv[optind++]);
            return -1;
        }
    }

    if (optind < argc) {
        printf("non-option ARGV-elements detected: ");
        while (optind < argc) {
            printf("%s ", argv[optind++]);
        }
        printf("\n");
        return -1;
    }

    /* Handle process_thread_masks */
    if (parse_csv_to_ul_array(cfg->process_thread_masks, 
                              cfg->process_thread_count,
                              opt_process_thread_masks) != 0) {
        return -1;
    }

//for (int i = 0; i < cfg->process_thread_count)

    /* realpath application log path */
    char *dir_realpath;

    dir_realpath = realpath(cfg->application_log_path, NULL);
    if (dir_realpath == NULL) {
        fprintf(stderr, "Error getting realpath for application log, path: %s, "
                " errno: %d, error message %s\n",
                cfg->application_log_path, errno, strerror(errno));
        return -1;
    }
    snprintf(cfg->application_log_realpath, FILE_REALPATH_MAX, "%s/%s",
            dir_realpath, cfg->application_log_name);
    free(dir_realpath);
    dir_realpath = NULL;

    /* realpath query log directory */    
    cfg->query_log_realpath = realpath( cfg->query_log_path, NULL);
    if (cfg->query_log_realpath == NULL) {
        fprintf(stderr, "Error getting realpath for query log, path: %s, "
                "errno: %d, error message %s\n",
                cfg->query_log_path, errno, strerror(errno));
        return -1;
    }

    return 0;
}

/** Clean configuration object. This will release memory assigned to object
 * parameters, not the object it self.
 * 
 * @param cfg Configuration object to clean.
 */
void
config_clean(config_t *cfg)
{
    free(cfg->process_thread_masks);
    free(cfg->application_log_name);
    free(cfg->application_log_path);
    free(cfg->query_log_base_name);
    free(cfg->query_log_path);

    free(cfg->resource_1_name);
    free(cfg->resource_1_filepath);

}

