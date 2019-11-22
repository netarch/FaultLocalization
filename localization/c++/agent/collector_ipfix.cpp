#include <iostream>
#include <stdio.h>
#include <thread>
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
#include <bayesian_net.h>

char collector_ip[] = "192.168.100.101";
int collector_port = 6000;

#define SIZE_FLOW_DATA_RECORD 52

using namespace std;

uint32_t ConvertStringIpToInt(string& ip_addr){
    //Take the last octet
    in_addr ip_address;
    if(inet_aton(ip_addr.c_str(), &ip_address) != 0){
        size_t index = ip_addr.find_last_of(".");
        //cout << "IP address " << ip_addr << " fixed32 " << ip_address.s_addr << " last_octet "
        //     << atoi(ip_addr.c_str() + index + 1) + OFFSET_HOST << " index " << index << endl;
        return atoi(ip_addr.c_str() + index + 1);
        //return ip_address.s_addr;
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

struct SocketThreadArgs{
    int socket;
    LogData *log_data;
};


void* SocketThread(void *arg){
    SocketThreadArgs* args = (SocketThreadArgs*)arg;
    int socket = args->socket;
    LogData *log_data = args->log_data;
    string data = "";
    recv_timeout(socket, data);

#ifndef IPFIX_EXPORT_FORMAT
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
        if (c_end-c_begin > 20 and reader.parse(c_data + c_begin, c_data + c_end + 1, root)){
            string src_ip = root.get("IPV4_SRC_ADDR", null_ip).asString();
            string dest_ip = root.get("IPV4_DST_ADDR", null_ip).asString();
            int packets_sent = root.get("OUT_PKTS", 0).asInt();
            int retransmissions = root.get("RETRANSMITTED_OUT_PKTS", 0).asInt();
            /* optional fields */
            int src_port = root.get("L4_SRC_PORT", 0).asInt();
            int dest_port = root.get("L4_DST_PORT", 0).asInt();
            int nbytes = root.get("OUT_BYTES", 0).asInt();
            int src_host = ConvertStringIpToInt(src_ip), dest_host = ConvertStringIpToInt(dest_ip);
            //cout << src_ip << " " << src_host << " " << dest_ip << " " << dest_host << endl;
            if (src_host > 0 and dest_host > 0 and packets_sent > 0){
                //cout << src_ip << " " << dest_ip << " " << packets_sent << " " << retransmissions
                //     << " flow_queue size: " << flow_queue.size() << endl;
                src_host += OFFSET_HOST;
                dest_host += OFFSET_HOST;
		assert(log_data->hosts_to_racks.find(src_host) != log_data->hosts_to_racks.end());
                int src_rack = log_data->hosts_to_racks[src_host];
		assert(log_data->hosts_to_racks.find(dest_host) != log_data->hosts_to_racks.end());
                int dest_rack = log_data->hosts_to_racks[dest_host];
		//cout << src_host << " " << src_rack << " " << dest_host << " " << dest_rack << endl;
                Flow *flow = new Flow(src_host, src_port, dest_host, dest_port, nbytes, 0.0);
		flow->SetFirstLinkId(log_data->GetLinkIdUnsafe(Link(flow->src, src_rack)));
		flow->SetLastLinkId(log_data->GetLinkIdUnsafe(Link(dest_rack, flow->dest)));
                flow->AddSnapshot(0.0, packets_sent, retransmissions, retransmissions);
		log_data->GetAllPaths(&flow->paths, src_rack, dest_rack);
		//!TODO: set path taken for all flows
		assert(flow->paths!=NULL and flow->paths->size() == 1);
		flow->SetPathTaken(flow->paths->at(0));
		//cout << "first_link_id " << flow->first_link_id << " last_link_id " << flow->last_link_id
	        //     << " flow->path_taken " << *flow->paths->at(0) << endl;
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
#else
    const char *c_data = data.c_str();
    size_t c_begin = 0;
    size_t c_end = data.length();

    while (c_begin < c_end) {
        u_int16_t message_length;
        memcpy(&message_length, c_data + c_begin + 2, sizeof(u_int16_t));
        int num_record = (message_length - 16 - 4) / SIZE_FLOW_DATA_RECORD;
        int i, offset;
        u_int16_t i16_tmp;
        u_int32_t i32_tmp;
        for (i = 0; i < num_record; ++i) {
            offset = 20 + i * SIZE_FLOW_DATA_RECORD;
            // IPV4_SRC_ADDR and L4_SRC_PORT.
            memcpy(&i32_tmp, c_data + c_begin + offset, sizeof(u_int32_t));
            memcpy(&i16_tmp, c_data + c_begin + offset + 8, sizeof(u_int16_t));
            string src_ip = intoa(i32_tmp);
            int src_port = (int)i16_tmp;
            // IPV4_DST_ADDR and L4_DST_PORT.
            memcpy(&i32_tmp, c_data + c_begin + offset + 4, sizeof(u_int32_t));
            memcpy(&i16_tmp, c_data + c_begin + offset + 10, sizeof(u_int16_t));
            string dest_ip = intoa(i32_tmp);
            int dest_port = (int)i16_tmp;
            // FIRST_SWITCHED.
            // memcpy(&i32_tmp, c_data + c_begin + offset + 36, sizeof(u_int32_t));
            // FIRST_SWITCHED_USEC.
            // memcpy(&i32_tmp, c_data + c_begin + offset + 40, sizeof(u_int32_t));
            // LAST_SWITCHED.
            // memcpy(&i32_tmp, c_data + c_begin + offset + 44, sizeof(u_int32_t));
            // LAST_SWITCHED_USEC.
            // memcpy(&i32_tmp, c_data + c_begin + offset + 48, sizeof(u_int32_t));
            // OUT_BYTES.
            memcpy(&i32_tmp, c_data + c_begin + offset + 12, sizeof(u_int32_t));
            int nbytes = (int)i32_tmp;
            // OUT_PKTS.
            memcpy(&i32_tmp, c_data + c_begin + offset + 16, sizeof(u_int32_t));
            int packets_sent = (int)i32_tmp;
            // RETRANSMITTED_OUT_BYTES.
            // memcpy(&i32_tmp, c_data + c_begin + offset + 20, sizeof(u_int32_t));
            // RETRANSMITTED_OUT_PKTS.
            // memcpy(&i32_tmp, c_data + c_begin + offset + 24, sizeof(u_int32_t));
            // MIN_DELAY_US.
            // memcpy(&i32_tmp, c_data + c_begin + offset + 28, sizeof(u_int32_t));
            // MAX_DELAY_US.
            // memcpy(&i32_tmp, c_data + c_begin + offset + 32, sizeof(u_int32_t));

            int src_host = ConvertStringIpToInt(src_ip);
            int dest_host = ConvertStringIpToInt(dest_ip);
            if (src_host > 0 and dest_host > 0 and packets_sent > 0){
                //cout << src_ip << " " << dest_ip << " " << packets_sent << " " << retransmissions
                //     << " flow_queue size: " << flow_queue.size() << endl;
                src_host += OFFSET_HOST;
                dest_host += OFFSET_HOST;
		assert(log_data->hosts_to_racks.find(src_host) != log_data->hosts_to_racks.end());
                int src_rack = log_data->hosts_to_racks[src_host];
		assert(log_data->hosts_to_racks.find(dest_host) != log_data->hosts_to_racks.end());
                int dest_rack = log_data->hosts_to_racks[dest_host];
		//cout << src_host << " " << src_rack << " " << dest_host << " " << dest_rack << endl;
                Flow *flow = new Flow(src_host, src_port, dest_host, dest_port, nbytes, 0.0);
		flow->SetFirstLinkId(log_data->GetLinkIdUnsafe(Link(flow->src, src_rack)));
		flow->SetLastLinkId(log_data->GetLinkIdUnsafe(Link(dest_rack, flow->dest)));
                flow->AddSnapshot(0.0, packets_sent, retransmissions, retransmissions);
		log_data->GetAllPaths(&flow->paths, src_rack, dest_rack);
		//!TODO: set path taken for all flows
		assert(flow->paths!=NULL and flow->paths->size() == 1);
		flow->SetPathTaken(flow->paths->at(0));
		//cout << "first_link_id " << flow->first_link_id << " last_link_id " << flow->last_link_id
	        //     << " flow->path_taken " << *flow->paths->at(0) << endl;
                flow_queue.push(flow);
            }
        }

        c_begin += message_length;
    }
#endif
    close(socket);
    delete args;
    pthread_exit(NULL);
}

void* RunAnalysisPeriodically(void* arg){
    LogData* log_data = (LogData*) arg;
    //!TODO: vipul
    BayesianNet estimator;
    vector<double> params = {1.0-5.0e-3, 2.0e-4};
    estimator.SetParams(params);
    int nopenmp_threads = 1;
    while(true){
        auto start_time = chrono::high_resolution_clock::now();
	    log_data->flows.clear();
        int nflows = flow_queue.size();
        for(int ii=0; ii<nflows; ii++){
            log_data->flows.push_back(flow_queue.pop());
        }
        //!TODO: call LocalizeFailures
	double max_finish_time_ms = 1000.0;
	estimator.SetLogData(log_data, max_finish_time_ms, nopenmp_threads);
	Hypothesis estimator_hypothesis;
	estimator.LocalizeFailures(0.0, max_finish_time_ms,
			           estimator_hypothesis, nopenmp_threads);
        double elapsed_time_ms = chrono::duration_cast<chrono::milliseconds>(
                              chrono::high_resolution_clock::now() - start_time).count();
	cout << "Output Hypothesis "  << log_data->IdsToLinks(estimator_hypothesis)
             << " analysis time taken for "<< nflows <<" flows (ms) " <<  elapsed_time_ms << endl;
	log_data->ResetForAnalysis();
        if (elapsed_time_ms < 1000.0){
	    cout << "Sleeping for time " << int(1000.0 - elapsed_time_ms)  << " ms" << endl;
	    chrono::milliseconds timespan(int(1000.0 - elapsed_time_ms));
	    std::this_thread::sleep_for(timespan);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]){
    ios_base::sync_with_stdio(false);

    char* topology_filename = argv[1];

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

    /* Get topology details from a file */
    LogData* log_data = new LogData();
    GetLinkMappings(topology_filename, log_data, true);

    /* Launch daemon thread that will periodically invoke the analysis */
    pthread_t tid;
    if(pthread_create(&tid, NULL, RunAnalysisPeriodically, log_data) != 0 ){
        perror("Failed to create analysis thread");
        exit(1);
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
            SocketThreadArgs *args = new SocketThreadArgs();
            args->socket = new_socket;
            args->log_data = log_data;
            if(pthread_create(&tid, NULL, SocketThread, args) != 0)
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
