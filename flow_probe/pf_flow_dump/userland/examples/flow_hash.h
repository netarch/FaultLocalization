#ifndef FLOW_HASH_
#define FLOW_HASH_

#define FLAG_SYN 0b00000010
#define FLAG_FIN 0b00000001

#define DEFAULT_HASH_SIZE 131072

// Parameter passed to alarm in unit of seconds.
#define DEFAULT_EXPORT_INTERVAL_SEC 1
// Long flow will push its stats for export when the next export time is within
// this interval (should be smaller than 10^6).
#define FLOW_STATS_PUSH_INTERVAL_USEC 100000


#define MAX_FLOW_PER_EXPORT 25
#define SIZE_FLOW_DATA_RECORD 52

struct SeqNoInFlight {
  u_int32_t pending_ack_no;
  struct timeval ts_sent;

  struct SeqNoInFlight* next;
};

struct Ipv4TcpFlowEntry {
  u_int32_t src_ip;
  u_int32_t dst_ip;

  u_int16_t src_port;
  u_int16_t dst_port;

  u_int32_t out_bytes;
  u_int32_t out_packets;

  u_int32_t retrans_bytes;
  u_int32_t retrans_packets;

  u_int32_t max_rtt_usec;
  u_int32_t min_rtt_usec;

  struct timeval ts_first;
  struct timeval ts_last;

  u_int64_t max_seq_no;

  struct Ipv4TcpFlowEntry* prev;
  struct Ipv4TcpFlowEntry* next;

  struct SeqNoInFlight* seq_no_in_flight_head;
  struct SeqNoInFlight* seq_no_in_flight_tail;

  struct timeval pending_export_ts;
};

/* Export IPFIX message format
    0               1               2               3
    0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |       FlowSet ID = 256        |      Length = 52 * N + 4      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       IPV4_SRC_ADDR (8)                       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       IPV4_DST_ADDR (8)                       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |        L4_SRC_PORT (7)        |        L4_DST_PORT (7)        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        OUT_BYTES (23)                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         OUT_PKTS (24)                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                RETRANSMITTED_OUT_BYTES (57600)                |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                 RETRANSMITTED_OUT_PKTS (57582)                |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       MIN_DELAY_US (??)                       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       MAX_DELAY_US (??)                       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                      FIRST_SWITCHED (22)                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    FIRST_SWITCHED_USEC (??)                   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       LAST_SWITCHED (21)                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    LAST_SWITCHED_USEC (??)                    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                              ...                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

#endif
