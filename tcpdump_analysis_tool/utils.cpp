#include "utils.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#define MAX_CHAR_PER_DUMP_LINE 1024
#define DEFAULT_HASH_SIZE 131072

// #define DEBUG_VERBOSE
#define TCPDUMP_CAPTURE
// #define PF_RING_CAPTURE

using namespace std;

vector<vector<FlowEntry> > flow_hash_map;

string get_tcp_flag_str(TCP_FLAGS flag) {
  switch (flag) {
    case SYN:
      return "SYN";
      break;
    case FIN:
      return "FIN";
      break;
    case ACK:
      return "ACK";
      break;
    case DATA:
      return "DATA";
      break;
    default:
      return "UNKOWN";
      break;
  }
}

string get_direction_str(DIRECTION direction) {
  switch (direction) {
    case TX:
      return "TX";
      break;
    case RX:
      return "RX";
      break;
    default:
      return "UNKOWN";
      break;
  }
}

bool flow_tuple_match(FlowEntry &flow_entry, PacketDumpInfo &dump_info) {
  // Treat a flow as bi-directional.
  if (flow_entry.direction == dump_info.direction) {
    return flow_entry.src_ip == dump_info.src_ip &&
        flow_entry.dst_ip == dump_info.dst_ip &&
        flow_entry.src_port == dump_info.src_port &&
        flow_entry.dst_port == dump_info.dst_port;
  } else {
    return flow_entry.src_ip == dump_info.dst_ip &&
        flow_entry.dst_ip == dump_info.src_ip &&
        flow_entry.src_port == dump_info.dst_port &&
        flow_entry.dst_port == dump_info.src_port;
  }
}

void ParsePacketDump(string line,
                       PacketDumpInfo &dump_info,
                       string *local_ip) {
#ifdef TCPDUMP_CAPTURE
  // Parse timestamp: <second.microsecond>.
  dump_info.ts_second = stoll(line.substr(0, line.find('.')));
  dump_info.ts_microsecond = stoll(line.substr(line.find('.') + 1, 6));

  // Parse source IP address and source port.
  int pos1 = line.find("IP ") + 3;
  int pos2 = line.find(" > ");
  string line_substr = line.substr(pos1, pos2 - pos1);
  pos1 = line_substr.find_last_of('.');
  dump_info.src_ip = line_substr.substr(0, pos1);
  dump_info.src_port = stoi(line_substr.substr(pos1 + 1));

  // Parse destination IP address and destination port.
  pos1 = pos2 + 3;
  pos2 = line.find(':');
  line_substr = line.substr(pos1, pos2 - pos1);
  pos1 = line_substr.find_last_of('.');
  dump_info.dst_ip = line_substr.substr(0, pos1);
  dump_info.dst_port = stoi(line_substr.substr(pos1 + 1));

  // Parse payload length.
  dump_info.length = stoi(line.substr(line.find("length") + 7));

  // Parse sequence number and ack number.
  pos1 = line.find("seq");
  if (pos1 != string::npos) {
    line_substr = line.substr(pos1 + 4);
    dump_info.seq_no = stol(dump_info.length == 0
        ? line_substr.substr(0, line_substr.find(','))
        : line_substr.substr(0, line_substr.find(':')));
  } else {
    // TODO: consider sack
    line_substr = line.substr(line.find("ack ") + 4);
    dump_info.ack_no = stol(line_substr.substr(0, line_substr.find(',')));
  }

  // Parse TCP flag.
  if (line.find("Flags [S") != string::npos) {
    dump_info.flag = SYN;
  } else if (line.find("Flags [F") != string::npos) {
    dump_info.flag = FIN;
  } else if (dump_info.length == 0) {
    dump_info.flag = ACK;
  } else {
    dump_info.flag = DATA;
  }

  // Parse trasmission direction: [TX/RX].
  if (dump_info.src_ip == *local_ip) {
    dump_info.direction = TX;
  } else {
    dump_info.direction = RX;
  }

  // Calculate packet hash.
  // Process source IP address.
  line_substr = dump_info.src_ip;
  pos1 = line_substr.find('.');
  while (pos1 != string::npos) {
    dump_info.packet_hash += stoi(line_substr.substr(0, pos1));
    line_substr = line_substr.substr(pos1 + 1);
    pos1 = line_substr.find('.');
  }
  dump_info.packet_hash += stoi(line_substr);
  // Process Destination IP address.
  line_substr = dump_info.dst_ip;
  pos1 = line_substr.find('.');
  while (pos1 != string::npos) {
    dump_info.packet_hash += stoi(line_substr.substr(0, pos1));
    line_substr = line_substr.substr(pos1 + 1);
    pos1 = line_substr.find('.');
  }
  dump_info.packet_hash += stoi(line_substr);
  // Process source and detination port.
  dump_info.packet_hash += 3 * (dump_info.src_port + dump_info.dst_port);
#endif
}

void ProcessDumpInfo(PacketDumpInfo &dump_info) {
  int hash_index = dump_info.packet_hash % DEFAULT_HASH_SIZE;
  bool found = false;
  int bucket_index = 0;
  for (bucket_index = 0; bucket_index < flow_hash_map[hash_index].size();
       ++bucket_index) {
    if (flow_tuple_match(flow_hash_map[hash_index][bucket_index],
                         dump_info)) {
      found = true;
      break;
    }
  }

  if (!found) {
    // Ignore flows started before tcpdump, as well as packets after FIN.
    if (dump_info.direction == RX || dump_info.flag != SYN) {
      return;
    }
    flow_hash_map[hash_index].push_back(FlowEntry(dump_info));
  }

  FlowEntry* flow_entry = &flow_hash_map[hash_index][bucket_index];
  if (dump_info.direction == TX && dump_info.length > 0) {
    flow_entry->out_bytes += dump_info.length;
    flow_entry->out_packets += 1;
    flow_entry->ts_second_last = dump_info.ts_second;
    flow_entry->ts_microsecond_last = dump_info.ts_microsecond;
    if (flow_entry->max_seq_no > dump_info.seq_no) {
      flow_entry->retrans_bytes += dump_info.length;
      flow_entry->retrans_packets += 1;
    } else {
      flow_entry->max_seq_no = dump_info.seq_no;
    }
  } else {
    // TODO: add RX packet processing for RTT update.
  }

  if (dump_info.flag == FIN && flow_entry->active) {
    flow_entry->active = false;
    // TODO: change to remote json transmit (or scp for simplicity)
    cout << flow_entry->src_ip << ":" << flow_entry->src_port << " "
         << flow_entry->dst_ip << ":" << flow_entry->dst_port << " ["
         << flow_entry->ts_second_first << "."
         << flow_entry->ts_microsecond_first << " -> "
         << flow_entry->ts_second_last << "."
         << flow_entry->ts_microsecond_last << "] "
         << flow_entry->out_bytes << "/"
         << flow_entry->out_packets << " ("
         << flow_entry->retrans_bytes << "/"
         << flow_entry->retrans_packets << ") "
         << "[" << get_tcp_flag_str(dump_info.flag) << "] "
         << endl;

    flow_hash_map[hash_index].erase(
        flow_hash_map[hash_index].begin() + bucket_index);
  }
}

// FIXME: local ip may fail for local flow
void StartTcpdump(string cmd, string local_ip) {
  FILE * rpipe = popen(cmd.c_str(), "r");
  char rbuffer[MAX_CHAR_PER_DUMP_LINE];
  flow_hash_map.resize(DEFAULT_HASH_SIZE);

  if (rpipe) {
    while (!feof(rpipe)) {
      if (fgets(rbuffer, MAX_CHAR_PER_DUMP_LINE, rpipe) != NULL) {
        string line(rbuffer);
#ifdef DEBUG_VERBOSE
        // cout << "read from pipe: [" << line << "] size "
        //      << line.size() << " ===> ";
#endif

        PacketDumpInfo dump_info;
        ParsePacketDump(line, dump_info, &local_ip);
#ifdef DEBUG_VERBOSE
        cout << dump_info.ts_second << "." << dump_info.ts_microsecond << " "
             << dump_info.src_ip << ":" << dump_info.src_port << " -> "
             << dump_info.dst_ip << ":" << dump_info.dst_port << " "
             << "(len: " << dump_info.length << ") "
             << "seq: " << dump_info.seq_no << " "
             << "ack: " << dump_info.ack_no << " "
             << "[" << get_tcp_flag_str(dump_info.flag) << "] "
             << "[" << get_direction_str(dump_info.direction) << "] "
             << "(hash: " << dump_info.packet_hash << ")"
             << endl;
#endif

        ProcessDumpInfo(dump_info);
      }
    }
    pclose(rpipe);
  }
}
