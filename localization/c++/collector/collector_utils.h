#ifndef __COLLECTOR_UTILS__
#define __COLLECTOR_UTILS__

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
#include "thread_pool.h"
#include "flow_parser.h"
/* Flock headers */
#include <flow.h>
#include <logdata.h>
#include <bayesian_net.h>

using namespace std;

void RunPeriodicAnalysisParams(FlowParser* flow_parser, Estimator &estimator,
                               vector<vector<double> > &params, vector<PDD> &result);

void* RunPeriodicAnalysisBayesianNet(void* arg);

void* RunPeriodicAnalysis007(void* arg);

void* RunPeriodicAnalysis(void* arg);

#endif
