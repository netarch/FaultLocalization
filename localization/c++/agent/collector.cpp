#include <iostream>
#include <stdio.h>
#include <algorithm>
#include <stdlib.h>
#include <jsoncpp/json/json.h>
#include <sys/socket.h>
#include <chrono>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <pthread.h>
#include <vector>

char collector_ip[] = "192.168.100.101";
int collector_port = 6000;

using namespace std;

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
    while (c_end != string::npos){
        //cout << "Parsing " << string(c_data+c_begin, c_end-c_begin+1) << endl;
        if (reader.parse(c_data + c_begin, c_data + c_end + 1, root)){
	    string src_ip = root["IPV4_SRC_ADDR"].asString();
	    string dest_ip = root["IPV4_DST_ADDR"].asString();
	    int packets_sent = root["OUT_PKTS"].asInt();
	    int retransmissions = root["RETRANSMITTED_OUT_PKTS"].asInt();
	    cout << src_ip << " " << dest_ip << " " << packets_sent << " " << retransmissions << endl;
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
    //pthread_t tid[60]; int i = 0;
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
            pthread_t tid;
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
