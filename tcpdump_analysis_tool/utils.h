#ifndef UTILS_H_
#define UTILS_H_

#include "flow_hash.h"

#include <string>
using namespace std;

string get_tcp_flag_str(TCP_FLAGS flag);
string get_direction_str(DIRECTION direction);

bool flow_tuple_match(PacketDumpInfo &dump_info1, PacketDumpInfo &dump_info2);

void ParsePacketDump(string line,
                       PacketDumpInfo &dump_info,
                       string *local_ip);
void ProcessDumpInfo(PacketDumpInfo &dump_info);

void StartTcpdump(string cmd, string local_ip);

#endif
