#ifndef __COLLECTOR_FLOW_PARSER__
#define __COLLECTOR_FLOW_PARSER__

#include <iostream>
#include <stdio.h>
#include <thread>
#include <algorithm>
#include <stdlib.h>
#include <sys/socket.h>
#include <chrono>
#include <netinet/in.h>
#include <string.h>
#include <mutex>
#include <arpa/inet.h>
#include <fstream>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <pthread.h>
#include <vector>
#include <queue>
/* Flock headers */
#include <flow.h>
#include <logdata.h>
#include <bayesian_net.h>

#define SIZE_FLOW_DATA_RECORD 52

using namespace std;

char *_intoa(unsigned int addr, char* buf, u_short bufLen);

static char *intoa(unsigned int addr) {
    static thread_local char buf[sizeof "ff:ff:ff:ff:ff:ff:255.255.255.255"];
    return(_intoa(addr, buf, sizeof(buf)));
}

uint32_t ConvertStringIpToInt(string& ip_addr);

void recv_timeout(int conn_socket, string &result);

struct FlowQueue{
    queue<Flow*> fq;
    mutex qlock;
    Flow* pop(){
        qlock.lock();
        Flow* flow = fq.front();
        fq.pop();
        qlock.unlock();
        return flow;
    }
    void push(Flow *flow){
        qlock.lock();
        fq.push(flow);
        qlock.unlock();
    }
    int size(){
        return fq.size();
    }
};

class FlowParser{
    map<array<int, 3>, Path*> path_taken_reference;
    FlowQueue flow_queue;
public:
    LogData* log_data;
    void HandleIncomingConnection(int socket);
    Path* GetPathTaken(int src_rack, int dst_rack, int dstport);
    FlowQueue* GetFlowQueue() { return &flow_queue; }
	void PreProcessPaths(string path_file);
    void PreProcessTopology(string topology_file);
};

#endif
