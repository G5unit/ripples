# Usage

        ripples [options]

        Options are:

        --help
                Print out this help message and exit.

        --udp_enable (True|False)
                Enable receiving DNS queries over UDP transport protocol.
                Default is True.

        --udp_listener_port (number 1-65535)
                Specify port to receive DNS queries over UDP transport protocol.
                Default port is 53.

        --udp_socket_recvbuff_size (number 518-16777215)
                Set size of UDP socket receive buffer
                Default size is 1048575.

                NOTE: Linux kernel setting net.core.rmem_max defines the upper cap of
                the maximum requestable buffer size. Importantly, this value must be
                greater than net.core.rmem_default.

        --udp_socket_sendbuff_size (number 512-16777215)
                Set size of UDP socket send buffer
                Default size is 1048575.

                NOTE: Linux kernel setting net.core.rmem_max defines the upper cap of
                the maximum requestable buffer size. Importantly, this value must be
                greater than net.core.rmem_default.

        --udp_conn_vector_len (number 1-65535)
                Specify vector length for recvmmsg() call. This sets the maximum
                number of UDP packets to read process per vectorloop
                Default is 8.

        --tcp_enable (True|False)
                Enable receiving DNS queries over TCP transport protocol.
                Default is True.

        --tcp_listener_pending_conns_max (number 1-65535)
                Specify how many pending connections TCP listener will keep in queue
                waiting to be accepted. WHen this number is reached any subsequent incoming
                TCP connection request will be rejected. This setting is passed as argument
                to system call listen().
                Default is 1024.

        --tcp_listener_port (number 1-65535)
                Specify port to receive DNS queries over TCP transport protocol.
                Default port is 53.

        --tcp_listener_max_accept_new_conn (number 1-65535)
                Specify how may new TCP connections should be accepted per Vectorloop
                iteration. Each Vectorloop iteration processes new queries received
                over UDP and existing TCP connections. In addition, new TCP connections
                are accepted. This parameter can be used to balance Vectorloop iteration
                time spent on each of these. Additional relevant options are
                "--tcp_conn_simultaneous_queries_count" and "--udp_conn_vector_len".
                Default is 8.

        --tcp_conn_socket_recvbuff_size (number 514-65535)
                Set size of TCP socket receive buffer
                Default size is 2048.

                NOTE: Linux kernel setting net.core.rmem_max defines the upper cap of
                the maximum requestable buffer size. Importantly, this value must be
                greater than net.core.rmem_default.

        --tcp_conn_socket_sendbuff_size (number 512-65535)
                Set size of UDP socket send buffer
                Default size is 12288.

                NOTE: Linux kernel setting net.core.rmem_max defines the upper cap of
                the maximum requestable buffer size. Importantly, this value must be
                greater than net.core.rmem_default.

        --tcp_conn_simultaneous_queries_count (number 1-255)
                A read from TCP socket could result in multiple queries read in. This
                setting limits number of queries that would be processed per read
                (vectorloop iteration) from a TCP connection.
                Default is 3.

        --tcp_keepalive (miliseconds 1000-600000)
                Time that TCP will stay IDLE, waiting for new queries to arrive. This
                timer is started after a query is processed, it does not apply when
                a connection is first established. When this timer is reached TCP connection
                is closed.
                Default is 10000 (10 seconds).

        --tcp_query_recv_timeout (miliseconds 1-UINTMAX_MAX)
                Time to wait for query to be received. This timer applies when a
                connection is first established, as well as when a partial query
                is received (waiting for bytes that completed the query request).
                When this timer is reached TCP connection is closed.
                Default is 2000 (2 seconds).

        --tcp_query_send_timeout (miliseconds 1-UINTMAX_MAX)
                Time to wait for TCP socket to become writable so a query response
                could be sent. Timer starts from first write attempt for a query and
                is reset when the full query is written to the TCP socket.
                When this timer is reached TCP connection is closed.
                Default is 2000 (2 seconds).

        --epoll_num_events_tcp (number 3-1024
                Maximum number of events to have reported in a single call to epoll.
                This settings includes TCP listeners and connections.
                Default is 8.

        --epoll_num_events_udp (number 3-1024
                Maximum number of events to have reported in a single call to epoll.
                This settings includes UDP listeners.
                Default is 8.

        --process_thread_count (number 1-1024)
                Set number of query processing (vectorloop) theads to start. This does
                ot include application support threads such as logging threads nor
                resource update thread. Number of support threads is fixed and not tunable.
                Default is 1.

        --process_thread_masks (no mask)
                Bind each vectorloop thread to a specific CPU. Format of this option
                value is a comma separated list of CPUs to bind each thread to. Index
                in value corresponds to thread number.
                I.E:
                --process_thread_masks=1,2,3,4,8,9
                sets CPU affinity for first six threads where thread 1 would bind
                to CPU1, thread 2 would bind to CPU2, .. , thread 5 would bind
                to CPU8, and thread 6 would bind to CPU9.
                If more bindings are provided than thread count (set by 
                --process_thread_count), the excess entries are ignored.
                If less bindings are provided than thread count, then the threads
                for which no CPU binding was provided would not have CPU affinity
                set and those threads would be subject to dynamic CPU scheduling
                underlying operating system uses.
                Default is no CPU affinity binding.

                Note that on Linux first CPU has ID of 0 (zero), where as this
                option starts numbering at 1

        --loop_slowdown_one (microseconds 1-10000)
                Vectorloop is a continuously running loop. If there are no queries to
                process the loop would needlessly consume CPU cycles. To avoid unnecessary
                consumption of CPU cycles vectorloop implements a slowdown policy where if
                no queries were processed in a cycle it sleeps for a specified time before
                continuing execution. The sleep time increases with each "idle" loop. The 
                policy is to sleep "loop_slowdown_one" time for 8 cycles, followed by
                "loop_slowdown_two" time for next 8 cycles, followed by "loop_slowdown_three"
                time until a query arrives. Policy is reset (start from the begining) each time
                vectorloop processed a query.
                Default is 1.

        --loop_slowdown_two (microseconds 1-10000)
                See description for option "loop_slowdown_one"
                Default is 100.

        --loop_slowdown_three (microseconds 1-10000)
                See description for option "loop_slowdown_one"
                Default is 50.

        --app_log_name (string)
                Name of application log file. See related optin "app_log_path".
                Maximum length of application log path plus name is 4096 which includes
                "/" between them.
                Default is "ripples.log".

        --app_log_path (string)
                Directory where to create application log file.
                Default is "." (directory where application is started in).

        --query_log_buffer_size (number 1-UINTMAX_MAX)
                Size in bytes for query log buffer. Each vectorloop allocates two
                buffers of this size used to write query logs to.
                Default is 6553500.

        --query_log_base_name (string)
                Base name for query log file. actual file name used is base name with
                appended timestamp in rfc3999nano format.
                Default is "dns_query_log".

        --query_log_path (string)
                Directory where to store query log files.
                Default is "logs", relative to directory application is started from.

        --query_log_rotate_size (number 1-UINTMAX_MAX)
                Size in bytes when exceeded a new query log file is opened. Note that this
                size limit is checked after each write (one write per vectorloop buffer)
                hence the actual file size would always exceed it.
                Default is 50000000.

        Example:
                ripples --udp_listener_port=9053 --tcp_enable=false
