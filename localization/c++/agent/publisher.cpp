#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include "flows.pb.h"
//#include "flow.h"

using namespace std;

uint32_t ConvertStringIpToInt(string& ip_addr){
    uint32_t ret = 0;
    size_t prev = 0;
    size_t pos = ip_addr.find_first_of('.');
    while(pos!=string::npos){
        ret = (ret << 8) + stoi(ip_addr.substr(prev, pos-prev));
        prev = pos+1;
        pos = ip_addr.find_first_of('.', prev);
    }
    ret = (ret << 8) + stoi(ip_addr.substr(prev));
    cout << "IP addr " << ip_addr << " fixed32 " << ret << endl;
    return ret;
}

void CreateProtoMessage(agent::Flows *flows, vector<agent::FlowInfo*> flow_infos){
    for (agent::FlowInfo* flow: flow_infos){
        agent::FlowInfo *flow_info = flows->add_flow_info();
        *flow_info = agent::FlowInfo(*flow);
    }
}

void CreateProtoFlow(agent::FlowInfo *flow_info, string flow_id, string src_ip, string dest_ip,
                        int packets_sent, int retransmissions){
    flow_info->set_flow_id(flow_id);
    flow_info->set_src_ip(ConvertStringIpToInt(src_ip));
    flow_info->set_dest_ip(ConvertStringIpToInt(dest_ip));
    flow_info->set_packets_sent(packets_sent);
    flow_info->set_retransmissions(retransmissions);
}

int main(int argc, char *argv[]){
    agent::FlowInfo *flow_info = new agent::FlowInfo();
    string flow_id = "flow";
    string src_ip = "192.168.3.101";
    string dest_ip = "192.168.3.102";
    int packets_sent = 1000, retransmissions=10;
    CreateProtoFlow(flow_info, flow_id, src_ip, dest_ip, packets_sent, retransmissions);

    vector<agent::FlowInfo*> flow_infos = {flow_info};
    agent::Flows* flows = new agent::Flows();
    CreateProtoMessage(flows, flow_infos);
    return 0;
}
