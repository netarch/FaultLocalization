#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <iostream>
#include <stdio.h>
#include <queue>
#include <unistd.h>
#include <pthread.h>
#include <malloc.h>
#include <stdlib.h>
#include <assert.h>
/* Flock headers */
#include <flow.h>
#include "flow_parser.h"

using namespace std;

void HandleIncomingConnection(int socket, FlowParser* flow_parser);

class ConnectionQueue{
public:
    ConnectionQueue() {
        pthread_mutex_init(&qmtx,0);
        // wcond is a condition variable that's signaled when new work arrives
        pthread_cond_init(&wcond, 0);
    }

    // Retrieve the next unattended connection from the queue
    int NextConnection() {
        pthread_mutex_lock(&qmtx);
        while (connections.empty()) pthread_cond_wait(&wcond, &qmtx);
        auto nc = connections.front();
        connections.pop();
        pthread_mutex_unlock(&qmtx);
        return nc;
    }

    void AddConnection(int socket_conn) {
        pthread_mutex_lock(&qmtx);
        connections.push(socket_conn);
        // signal there's new work
        pthread_cond_signal(&wcond);
        pthread_mutex_unlock(&qmtx);
    }

    void SetFlowParser(FlowParser* flow_parser_) { flow_parser = flow_parser_; }
    FlowParser* GetFlowParser() { return flow_parser; }

private:
    std::queue<int> connections;
    FlowParser* flow_parser=NULL;
    pthread_mutex_t qmtx;
    pthread_cond_t wcond;
};

inline void* ThreadProcess(void* param){
    ConnectionQueue* cq = reinterpret_cast<ConnectionQueue*>(param);
    while (true){
        int conn_socket = cq->NextConnection();
        HandleIncomingConnection(conn_socket, cq->GetFlowParser());
	close(conn_socket);
    }
}


class ThreadPool {
public:
    // Allocate a thread pool and set them to decode connections
    ThreadPool(int n) {
        cout << "Creating thread pool with " << n << " threads" << endl;
        handles.resize(n);
        for (int i=0; i<n; ++i){
            if (pthread_create(&handles[i], NULL, ThreadProcess, &conn_queue)) exit(0);
        }
    }
    void SetFlowParser(FlowParser* flow_parser) { conn_queue.SetFlowParser(flow_parser); }
    void AddConnection(int socket_conn) { conn_queue.AddConnection(socket_conn); }
private:
    std::vector<pthread_t> handles;
    ConnectionQueue conn_queue;
};

#endif
