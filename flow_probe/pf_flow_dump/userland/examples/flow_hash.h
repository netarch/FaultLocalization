#ifndef FLOW_HASH_
#define FLOW_HASH_

#define FLAG_SYN 0b00000010
#define FLAG_FIN 0b00000001

#define DEFAULT_HASH_SIZE 131072

struct Ipv4TcpFlowEntry {
  u_int32_t src_ip;
  u_int32_t dst_ip;

  u_int16_t src_port;
  u_int16_t dst_port;

  u_int32_t out_bytes;
  u_int32_t out_packets;

  u_int32_t retrans_bytes;
  u_int32_t retrans_packets;

  struct timeval ts_first;
  struct timeval ts_last;

  u_int32_t max_seq_no;

  struct Ipv4TcpFlowEntry* prev;
  struct Ipv4TcpFlowEntry* next;
};

#endif
