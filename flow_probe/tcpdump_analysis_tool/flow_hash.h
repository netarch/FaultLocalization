#ifndef FLOW_HASH_
#define FLOW_HASH_

#include <string>
using namespace std;

enum TCP_FLAGS {SYN, FIN, ACK, DATA};
enum DIRECTION {TX, RX};

// TODO: add UDP support.
// TODO: add IPv6 support.
struct PacketDumpInfo {
  PacketDumpInfo()
      : seq_no(-1), ack_no(-1), length(0), flag(DATA), packet_hash(0) {}

  long long ts_second;
  long long ts_microsecond;

  string src_ip;
  string dst_ip;

  int src_port;
  int dst_port;

  int length;

  long seq_no;
  long ack_no;

  TCP_FLAGS flag;
  DIRECTION direction;

  u_int32_t packet_hash;
};

struct FlowEntry {
  FlowEntry(PacketDumpInfo& dump_info) :
      src_ip(dump_info.src_ip),
      dst_ip(dump_info.dst_ip),
      src_port(dump_info.src_port),
      dst_port(dump_info.dst_port),
      direction(dump_info.direction),
      out_bytes(0),
      out_packets(0),
      retrans_bytes(0),
      retrans_packets(0),
      ts_second_first(dump_info.ts_second),
      ts_microsecond_first(dump_info.ts_microsecond),
      ts_second_last(dump_info.ts_second),
      ts_microsecond_last(dump_info.ts_microsecond),
      max_seq_no(-1),
      active(true) {}

  string src_ip;
  string dst_ip;

  int src_port;
  int dst_port;

  DIRECTION direction;

  long out_bytes;
  long out_packets;

  long retrans_bytes;
  long retrans_packets;

  long long ts_second_first;
  long long ts_microsecond_first;
  long long ts_second_last;
  long long ts_microsecond_last;

  long max_seq_no;

  bool active;
};

#endif
