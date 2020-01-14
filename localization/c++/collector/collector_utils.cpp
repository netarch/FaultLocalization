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
#include "collector_utils.h"
/* Flock headers */
#include <flow.h>
#include <doubleO7.h>
#include <logdata.h>
#include <bayesian_net.h>

using namespace std;

void RunPeriodicAnalysisParams(FlowParser* flow_parser, Estimator &estimator, vector<vector<double> > &params, vector<PDD> &result){
    result.clear();
    LogData* log_data = flow_parser->log_data;
    FlowQueue* flow_queue = flow_parser->GetFlowQueue();
    uint32_t period_ms = 10000;
    Hypothesis failed_links_set;
    log_data->GetFailedLinkIds(failed_links_set);
    int nopenmp_threads = 1;
    uint16_t K = 20;
    vector<PDD> prec_recall_vec;
    for(int ii=0; ii<params.size(); ii++){
        estimator.SetParams(params[ii]);
        prec_recall_vec.clear();
        while(prec_recall_vec.size() < K){
            auto start_time = chrono::high_resolution_clock::now();
            log_data->flows.clear();
            int nflows = flow_queue->size();
            for(int ii=0; ii<nflows; ii++){
                log_data->flows.push_back(flow_queue->pop());
            }
            double max_finish_time_ms = period_ms;
            estimator.SetLogData(log_data, max_finish_time_ms, nopenmp_threads);
            Hypothesis estimator_hypothesis;
            estimator.LocalizeFailures(0.0, max_finish_time_ms,
                                      estimator_hypothesis, nopenmp_threads);
            PDD prec_recall = GetPrecisionRecall(failed_links_set, estimator_hypothesis);
            if (nflows > 12000){
                /* Update precision recall metrics */
                prec_recall_vec.push_back(prec_recall);
                log_data->ResetForAnalysis();
            }
            double elapsed_time_ms = chrono::duration_cast<chrono::milliseconds>(
                    chrono::high_resolution_clock::now() - start_time).count();
            cout << "Iteration: " << prec_recall_vec.size() << ", output Hypothesis "  << log_data->IdsToLinks(estimator_hypothesis)
                 << ", analysis time: "<< elapsed_time_ms << " ms, flows: " << nflows << ", precision,recall " << prec_recall;
            if (elapsed_time_ms < period_ms){
                cout << ", sleeping for " << int(period_ms - elapsed_time_ms)  << " ms" << endl;
                chrono::milliseconds timespan(int(period_ms - elapsed_time_ms));
                std::this_thread::sleep_for(timespan);
            }
            else cout << endl;
        }
        PDD prec_recall_avg(0.0, 0.0);
        for (auto elt: prec_recall_vec) prec_recall_avg = prec_recall_avg + elt;
        prec_recall_avg.first /= prec_recall_vec.size();
        prec_recall_avg.second /= prec_recall_vec.size();
        result.push_back(prec_recall_avg);
	cout << "RunPeriodicAnalysisParams " << params[ii] << " " << prec_recall_avg << endl;
    }
}

void* RunPeriodicAnalysis007(void* arg){
    FlowParser* flow_parser = (FlowParser*) arg;
    vector<vector<double> > params;
    double min_fail_threshold = 0.21; //0.01;
    double max_fail_threshold = 0.26; //0.10;
    double step = 0.005; //0.01
    for (double fail_threshold=min_fail_threshold;
                fail_threshold < max_fail_threshold; fail_threshold += step){
        params.push_back(vector<double> {fail_threshold});
    }
    //params = {{0.1}, {0.2}};
    DoubleO7 estimator;
    vector<PDD> result;
    RunPeriodicAnalysisParams(flow_parser, estimator, params, result);
    int ctr = 0;
    for (auto &param: params){
        auto p_r = result[ctr++];
        cout << param << " " << p_r << endl;
    }
    return NULL;
}

void* RunPeriodicAnalysisBayesianNet(void* arg){
    FlowParser* flow_parser = (FlowParser*) arg;
    vector<vector<double> > params;
    double eps = 1.0e-10;
    for (double p1c = 1.0e-3; p1c <=5e-3+eps; p1c += 1.0e-3){
        for (double p2 = 1.0e-4; p2 <= 4.0e-4+eps; p2 += 0.5e-4){
            if (p2 >= p1c - 0.25e-4) continue;
            double nprior = 25.0;
            //for(double nprior=5.0; nprior<=35.0+eps; nprior+=10.0){
                params.push_back(vector<double> {1.0 - p1c, p2, -nprior});
            //}
        }
    }
    params = {{1.0 - 5.0e-3, 2.0e-4, -100.0}
	     ,{1.0 - 5.0e-3, 4.0e-4, -100.0}
	     ,{1.0 - 5.0e-3, 5.0e-4, -100.0}
	     ,{1.0 - 6.0e-3, 5.0e-4, -100.0}
	      };		
    BayesianNet estimator;
    vector<PDD> result;
    RunPeriodicAnalysisParams(flow_parser, estimator, params, result);
    int ctr = 0;
    for (auto &param: params){
        param[0] = 1.0 - param[0];
        auto p_r = result[ctr++];
        cout << param << " " << p_r << endl;
    }
    return NULL;
}

void* RunPeriodicAnalysis(void* arg){
    cout << "Periodic analysis for Bayesian net " << endl;
    RunPeriodicAnalysisBayesianNet(arg);
    cout << "Periodic analysis for 007 " << endl;
    RunPeriodicAnalysis007(arg);
}
