#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fstream>
#include <sstream>
#include <vector>
#include "flows.pb.h"
//#include "flow.h"

using namespace std;

int SendToAgent(agent::Flows *flows, string agent_ip_address, int agent_port){
    int sock = 0; 
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){ 
        cout << "Socket creation error, failed sending to agent" << endl; 
        return -1; 
    } 
   
    struct sockaddr_in agent_address; 
    agent_address.sin_family = AF_INET; 
    agent_address.sin_port = htons(agent_port); 
    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, agent_ip_address.c_str(), &agent_address.sin_addr)<=0){ 
		cout << "Invalid/unsupported address" << endl;
        return -1; 
    } 
  
    cout << "SendToAgent " << agent_ip_address << ":" << agent_port << " " << inet_ntoa(agent_address.sin_addr) << " " << sock << endl;
    if (connect(sock, (struct sockaddr *)&agent_address, sizeof(agent_address)) < 0){ 
		cout << "Connection with agent failed" << endl;
        return -1; 
    } 
    string data;
    flows->SerializeToString(&data);
    uint32_t data_size = data.size();
    cout << "Length of serialized data " << data_size << endl;
    char *buffer = new char[sizeof(uint32_t) + data_size];
    memcpy(buffer, &data_size, sizeof(data_size)); 
    memcpy(buffer+sizeof(data_size), data.c_str(), data_size);
    send(sock, buffer, sizeof(uint32_t) + data_size, 0);
}



uint32_t ConvertStringIpToInt(string& ip_addr){
    in_addr ip_address;
    if(inet_aton(ip_addr.c_str(), &ip_address) != 0){
        cout << "IP address " << ip_addr << " fixed32 " << ip_address.s_addr << endl;
        return ip_address.s_addr;
    }
    else{
        cout << "Ip address " << ip_addr << " is not valid." << endl;
        exit(1);
    }
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
	assert(argc == 3);
	string agent_ip_address = argv[1];
	int agent_port = atoi(argv[2]);
    agent::FlowInfo *flow_info = new agent::FlowInfo();
    string flow_id = "flow";
    string src_ip = "192.168.3.101";
    string dest_ip = "192.168.3.102";
    int packets_sent = 1000, retransmissions=10;
    CreateProtoFlow(flow_info, flow_id, src_ip, dest_ip, packets_sent, retransmissions);

    vector<agent::FlowInfo*> flow_infos = {flow_info};
    agent::Flows* flows = new agent::Flows();
    CreateProtoMessage(flows, flow_infos);
	SendToAgent(flows, agent_ip_address, agent_port);
    return 0;
}

