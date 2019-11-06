#include <iostream>
#include <unistd.h> 
#include <stdio.h> 
#include <stdint.h>
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <string.h> 
#include "flows.pb.h"

using namespace std;

const int AGENT_PORT = 20000;


string ConvertIntIpToString(uint32_t ip_addr_int){
   in_addr ip_address;
   ip_address.s_addr = ip_addr_int;
   //!TODO inet_ntoa is not thread_safe!
   char *char_ip = inet_ntoa(ip_address);
   string ret(char_ip);
   return ret;
}

void ReadDataFromBuffer(agent::Flows *flows, int socket){
    uint32_t received_size;
    read(socket, &received_size, sizeof(received_size));
    cout << "received_size " << received_size << endl;
    string input_string;
    input_string.resize(received_size);
    read(socket, input_string.data(), received_size);
    flows->ParseFromString(input_string);
    for (const agent::FlowInfo& flow: flows->flow_info()){
        cout << "Flow id " << flow.flow_id() << " src_ip " << ConvertIntIpToString(flow.src_ip())
             << " dest_ip " << ConvertIntIpToString(flow.dest_ip()) << " packets sent " 
             << flow.packets_sent() << " retransmissions " << flow.retransmissions() << endl;
    }
}

int main(int argc, char const *argv[]) { 
    int server_fd, sock_opt=1; 
    // Create socket file descriptor 
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) { 
        perror("error while creating socket"); 
        exit(1); 
    } 
    // Set some options on the socket
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &sock_opt, sizeof(sock_opt)) != 0){ 
        perror("error while setting socket options"); 
        exit(1); 
    } 

    sockaddr_in agent_address; 
    bzero((char*)&agent_address, sizeof(agent_address));
    agent_address.sin_family = AF_INET; 
    //agent_address.sin_addr.s_addr = INADDR_ANY; 
    agent_address.sin_addr.s_addr = htonl(INADDR_ANY);
    agent_address.sin_port = htons(AGENT_PORT); 
       
    // Attach socket to random port
    if (bind(server_fd, (struct sockaddr *)&agent_address, sizeof(agent_address)) != 0){ 
        perror("error while binding"); 
        exit(1); 
    }

    // Get agent port number
	socklen_t struct_size = sizeof(sockaddr_in);
	if (getsockname(server_fd, (struct sockaddr *)&agent_address, &struct_size) != 0){
		cout << "failed to get hostname with errno: "<< errno << endl;
		exit(1);
	}
	cout << "Listening on port " << ntohs(agent_address.sin_port) << endl;

    while(true){
        cout << "Listening " << endl;
        // Listen on socket for incomming connections
        const int MAX_PENDING_CONNECTIONS = 3;
        if (listen(server_fd, MAX_PENDING_CONNECTIONS) != 0){ 
            perror("error while listening on socket"); 
            exit(1); 
        }
        int new_socket, addrlen = sizeof(agent_address); 
        if ((new_socket = accept(server_fd, (struct sockaddr *)&agent_address,  
                                 (socklen_t*)&addrlen))<0){ 
            perror("error while accepting connection"); 
            exit(1); 
        } 
        cout << "Listening " << endl;
        agent::Flows *flows = new agent::Flows();
        ReadDataFromBuffer(flows, new_socket);
    }
    return 0; 
} 
