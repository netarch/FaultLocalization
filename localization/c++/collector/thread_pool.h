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
#include <logdata.h>
#include <bayesian_net.h>

using namespace std;

void HandleIncomingConnection(int socket, LogData *log_data);

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

    void SetLogData(LogData* log_data_) { log_data = log_data_; }
    LogData* GetLogData() { return log_data; }

private:
    std::queue<int> connections;
    LogData* log_data=NULL;
    pthread_mutex_t qmtx;
    pthread_cond_t wcond;
};


void* ThreadProcess(void* param){
    ConnectionQueue* cq = reinterpret_cast<ConnectionQueue*>(param);
    while (true){
        int conn_socket = cq->NextConnection();
        HandleIncomingConnection(conn_socket, cq->GetLogData());
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
    void SetLogData(LogData* log_data) { conn_queue.SetLogData(log_data); }
    void AddConnection(int socket_conn) { conn_queue.AddConnection(socket_conn); }
private:
    std::vector<pthread_t> handles;
    ConnectionQueue conn_queue;
};
