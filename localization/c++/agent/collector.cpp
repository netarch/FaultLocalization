#include <iostream>
#include <stdio.h>
#include <algorithm>
#include <stdlib.h>
#include <jsoncpp/json/json.h>
#include <sys/socket.h>
#include <chrono>
#include <netinet/in.h>
#include <string.h>
#include <mutex>
#include <arpa/inet.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <pthread.h>
#include <vector>
#include <queue>
/* Flock headers */
#include <flow.h>
#include <logdata.h>

char collector_ip[] = "192.168.100.101";
int collector_port = 6000;

using namespace std;

uint32_t ConvertStringIpToInt(string& ip_addr){
    in_addr ip_address;
    if(inet_aton(ip_addr.c_str(), &ip_address) != 0){
        //cout << "IP address " << ip_addr << " fixed32 " << ip_address.s_addr << endl;
        return ip_address.s_addr;
    }
    else{
        cout << "Ip address " << ip_addr << " is not valid." << endl;
        return 0;
    }
}

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
FlowQueue flow_queue;

void recv_timeout(int conn_socket, string &result){
    // Put the socket in non-blocking mode:
    if(fcntl(conn_socket, F_SETFL, fcntl(conn_socket, F_GETFL) | O_NONBLOCK) < 0) {
        cerr << "Unable to put socket in non-blocking mode. Abandoning connection " << endl;
        return;
    }
    float timeout_ms = 10.0;
    float sleep_after_recv_sec = 1.0e-3;
    auto begin_time = std::chrono::system_clock::now();
    result.clear();
    char buffer[1024];
    while(true){
        double elapsed_time_ms = chrono::duration_cast<chrono::milliseconds>(
                              chrono::high_resolution_clock::now() - begin_time).count();
        if (result.size() > 0 and elapsed_time_ms > timeout_ms) break;
        else if (elapsed_time_ms > 2 * timeout_ms) break;
        else{
            int bytes = recv(conn_socket, buffer, 1024, 0);
            if (bytes > 0){
                result.append(buffer, bytes);
                begin_time = std::chrono::system_clock::now();
            }
            else{
                sleep(sleep_after_recv_sec);
            }
        }
    }
}

void* SocketThread(void *arg){
    int socket = *((int *)arg);
    string data = "";
    recv_timeout(socket, data);
    /* Sample JSON
    {"IPV4_SRC_ADDR":"192.168.100.108",
     "IPV4_DST_ADDR":"192.168.100.101",
     "PROTOCOL":6,
     "L4_SRC_PORT":44692,
     "L4_DST_PORT":6000,
     "FIRST_SWITCHED":1573805783,
     "LAST_SWITCHED":1573805783,
     "OUT_PKTS":4,
     "OUT_BYTES":216,
     "RETRANSMITTED_IN_BYTES":0,
     "RETRANSMITTED_OUT_PKTS":0,
     "CLIENT_NW_LATENCY_MS":0.062,
     "SERVER_NW_LATENCY_MS":0.018,
     "APPL_LATENCY_MS":0.000,
     "TOTAL_FLOWS_EXP":1160}
    */

    // remove newlines
    data.erase(remove(data.begin(), data.end(), '\n'), data.end());
    //cout << "Received " << data << " Finished received, size " << data.size() << endl;
    const char *c_data = data.c_str();
    size_t c_begin = 0;
    size_t c_end = data.find_first_of('}', 0);
    Json::Reader reader;
    Json::Value root;
    string null_ip = "0.0.0.0"; //verified that when converted to integer is 0
    assert (ConvertStringIpToInt(null_ip) == 0);
    while (c_end != string::npos){
        //cout << "Parsing " << string(c_data+c_begin, c_end-c_begin+1) << endl;
        if (c_end-c_begin > 1 and reader.parse(c_data + c_begin, c_data + c_end + 1, root)){
            string src_ip = root.get("IPV4_SRC_ADDR", null_ip).asString();
            string dest_ip = root.get("IPV4_DST_ADDR", null_ip).asString();
            int packets_sent = root.get("OUT_PKTS", 0).asInt();
            int retransmissions = root.get("RETRANSMITTED_OUT_PKTS", 0).asInt();
            /* optional fields */
            int src_port = root.get("L4_SRC_PORT", 0).asInt();
            int dest_port = root.get("L4_DST_PORT", 0).asInt();
            int nbytes = root.get("OUT_BYTES", 0).asInt();
            int src_ip_int = ConvertStringIpToInt(src_ip), dest_ip_int = ConvertStringIpToInt(dest_ip);
            if (src_ip_int > 0 and dest_ip_int > 0 and packets_sent > 0){
                //cout << src_ip << " " << dest_ip << " " << packets_sent << " " << retransmissions
                //     << " flow_queue size: " << flow_queue.size() << endl;
                Flow *flow = new Flow(src_ip_int, src_ip, src_port, dest_ip_int, dest_ip, dest_port, nbytes, 0.0);
                flow->AddSnapshot(0.0, packets_sent, retransmissions, retransmissions);
                flow_queue.push(flow);
            }
        }
        else{
            perror("Error parsing received json info");
            break;
        }
        c_begin = c_end+1;
        c_end = data.find_first_of('}', c_begin);
    }
    close(socket);
    pthread_exit(NULL);
}

void* RunPeriodicAnalysis(void* arg){
    while(true){
        auto start_time = chrono::high_resolution_clock::now();
        LogData log_data;
        int nflows = flow_queue.size();
        for(int ii=0; ii<nflows; ii++){
            log_data.flows.push_back(flow_queue.pop());
        }
        double elapsed_time_ms = chrono::duration_cast<chrono::milliseconds>(
                              chrono::high_resolution_clock::now() - start_time).count();
        cout << "Time taken for analysis on "<< nflows <<" nflows (ms) " <<  elapsed_time_ms << endl;
        if (elapsed_time_ms < 1000.0){
            sleep(1.0 - elapsed_time_ms*1.0e-3);
        }
    }
    return NULL;
}

int main(){
    ios_base::sync_with_stdio(false);

    int collector_socket, sock_opt=1;
    // Create socket file descriptor 
    if ((collector_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Error while creating socket");
        exit(1); 
    } 

    // Set some options on the socket
    if (setsockopt(collector_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &sock_opt, sizeof(sock_opt)) != 0){
        perror("Error while setting socket options");
        exit(1);
    } 

    sockaddr_in collector_address;
    bzero((char*)&collector_address, sizeof(collector_address));
    collector_address.sin_family = AF_INET;
    //!TODO: see if " = htonl () " works
    collector_address.sin_addr.s_addr = inet_addr(collector_ip);
    collector_address.sin_port = htons(collector_port);

    if (bind(collector_socket, (struct sockaddr *)&collector_address, sizeof(collector_address)) != 0){
        perror("Error while binding");
        exit(1);
    }
    /* Launch daemon thread that will periodically invoke the analysis */
    pthread_t tid;
    if(pthread_create(&tid, NULL, RunPeriodicAnalysis, NULL) != 0 ){
        cout << "Failed to create analysis thread" << endl;
        exit(0);
    }
    while(true){
        int MAX_PENDING_CONNECTIONS = 128; 
        if (listen(collector_socket, MAX_PENDING_CONNECTIONS) != 0){
            perror("Error while listening on socket");
            continue;
        }

        int new_socket, addrlen = sizeof(collector_address);
        if ((new_socket = accept(collector_socket, (struct sockaddr *)&collector_address,
                        (socklen_t*)&addrlen))<0){
            perror("Error while accepting connection");
            continue;
        }
        else{
            if(pthread_create(&tid, NULL, SocketThread, &new_socket) != 0 )
                cout << "Failed to create thread" << endl;
        }
        /*
        if(i >= 50){
            i = 0;
            while(i < 50) {
                pthread_join(tid[i++],NULL);
            }
            i = 0;
        }
        */
    }
    return 0;
}
