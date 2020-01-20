#ifndef FLOW_HASH_
#define FLOW_HASH_

#define DEFAULT_HASH_SIZE 131072

#define DEFAULT_EXPORT_INTERVAL_SEC 5

#define MAX_FLOW_PER_EXPORT 25
#define SIZE_FLOW_DATA_RECORD 52

struct Ipv4TcpFlowEntry {
  u_int32_t src_ip;
  u_int32_t dst_ip;

  u_int16_t src_port;
  u_int16_t dst_port;

  u_int32_t out_bytes;
  u_int32_t out_packets;

  u_int32_t retrans_bytes;
  u_int32_t retrans_packets;

  u_int32_t out_bytes_snapshot;
  u_int32_t out_packets_snapshot;

  u_int32_t retrans_bytes_snapshot;
  u_int32_t retrans_packets_snapshot;

  u_int32_t srtt_usec;

  struct Ipv4TcpFlowEntry* prev;
  struct Ipv4TcpFlowEntry* next;

  unsigned long long version;
};

struct FlowEntryWrapper {
  struct Ipv4TcpFlowEntry* entry;

  struct FlowEntryWrapper* next;
};

#endif
