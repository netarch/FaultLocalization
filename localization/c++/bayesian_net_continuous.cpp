#include "bayesian_net_continuous.h"
#include "utils.h"
#include <assert.h>
#include <numeric>
#include <algorithm>
#include <queue>
#include <chrono>
#include <cmath>
#include <iostream>
#include <omp.h>
#include <tuple>
#include <sparsehash/dense_hash_set> 
using google::dense_hash_set;
using namespace std;

BayesianNetContinuous* BayesianNetContinuous::CreateObject(){
    BayesianNetContinuous* ret = new BayesianNetContinuous();
    //!TODO : set params
    //ret->SetParams(param);
    return ret;
}

//!TODO: implement
void BayesianNetContinuous::SetParams(vector<double>& param) {
    assert(false);
    assert(param.size() == 0);
}
    
void BayesianNetContinuous::SetLogData(LogData* data_, double max_finish_time_ms, int nopenmp_threads){
    Estimator::SetLogData(data_, max_finish_time_ms, nopenmp_threads);
    if(USE_CONDITIONAL){
        data->FilterFlowsForConditional(max_finish_time_ms, nopenmp_threads);
    }
}

bool BayesianNetContinuous::DiscardFlow(Flow *flow, double min_start_time_ms, double max_finish_time_ms){
    return (flow->start_time_ms<min_start_time_ms or !flow->AnySnapshotBefore(max_finish_time_ms)
           or (USE_CONDITIONAL and !flow->TracerouteFlow(max_finish_time_ms)));
}

long double BayesianNetContinuous::FlowProbabilityForPath(int weight_good,
                                    int weight_bad, double path_fail_rate){
    return pow((long double)path_fail_rate, weight_bad)
         * pow((long double)(1.0 - path_fail_rate), weight_good);
}

long double BayesianNetContinuous::PathGradientNumerator(int weight_good,
                                   int weight_bad, double path_fail_rate){
    if(weight_bad==0 and weight_good==0) return 0;
    else if(weight_bad==0)return -weight_good * pow((long double)(1.0 - path_fail_rate), weight_good-1);
    else if(weight_good==0) return weight_bad * pow((long double)path_fail_rate, weight_bad - 1);
    else return pow((long double)path_fail_rate, weight_bad-1)
              * pow((long double)(1.0 - path_fail_rate), weight_good-1)
              * (weight_bad * (1.0 - path_fail_rate) - weight_good * path_fail_rate);
}

double BayesianNetContinuous::PathFailRate(int first_link_id, int last_link_id,
                                        Path *path, vector<double> &loss_rates){
    double path_success_rate = (1.0-loss_rates[first_link_id]) * (1.0-loss_rates[last_link_id]);
    for (int link_id: *path){
        path_success_rate *= (1.0-loss_rates[link_id]);
    }
    return 1.0-path_success_rate;
}


void BayesianNetContinuous::LocalizeFailures(double min_start_time_ms, double max_finish_time_ms,
                          Hypothesis &localized_links, int nopenmp_threads){
    int nlinks = data->inverse_links.size();
    vector<double> loss_rates(nlinks, initial_loss_rate);
    vector<double> gradients(nlinks, 0.0);
    long double log_likelihood;
    int iter=0, niters = 100;
    while (++iter < niters){
        log_likelihood = ComputeLogLikelihood(loss_rates, min_start_time_ms,
                                            max_finish_time_ms, nopenmp_threads);
        if constexpr (VERBOSE) {
            cout << "Iteration " << iter << " log likelihood " << log_likelihood << endl;
        }
        ComputeGradients(gradients, loss_rates, min_start_time_ms, max_finish_time_ms, nopenmp_threads);
        for (int link_id=0; link_id<nlinks; link_id++){
            loss_rates[link_id] -= learning_rate * gradients[link_id];
            if (loss_rates[link_id]>1.0e-3){
                cout<<"Failed link "<< data->inverse_links[link_id]<< " "<< loss_rates[link_id]<<endl;
            }
        }
    }
}


long double BayesianNetContinuous::ComputeLogLikelihood(vector<double> &loss_rates,
              double min_start_time_ms, double max_finish_time_ms, int nopenmp_threads){
    assert(loss_rates.size() == data->inverse_links.size());
    vector<long double> likelihood_threads(nopenmp_threads, 0.0);
    //!TODO implement USE_CONDITIONAL
    assert(!USE_CONDITIONAL);
    #pragma omp parallel for num_threads(nopenmp_threads)
    for (int ff=0; ff<data->flows.size(); ff++){
        Flow* flow = data->flows[ff];
        if (DiscardFlow(flow, min_start_time_ms, max_finish_time_ms)) continue;
        vector<Path*>* flow_paths = flow->GetPaths(max_finish_time_ms);
        int npaths = flow_paths->size();
        static_assert(!CONSIDER_REVERSE_PATH);
        PII weight = flow->LabelWeightsFunc(max_finish_time_ms);
        if (weight.first == 0 and weight.second == 0) continue;
        long double flow_prob = 0.0;
        for (Path* path: *flow_paths){
            double path_fail_rate = PathFailRate(flow->first_link_id, flow->last_link_id,
                                                 path, loss_rates);
            flow_prob += FlowProbabilityForPath(weight.first, weight.second, path_fail_rate)/npaths;
        }
        int thread_num = omp_get_thread_num();
        flow->SetCachedIntermediateValue(flow_prob);
        likelihood_threads[thread_num] += log(flow_prob);
    }
    long double log_likelihood = accumulate(likelihood_threads.begin(),
                                            likelihood_threads.end(), 0.0, plus<double>());
    return log_likelihood;
}


void BayesianNetContinuous::ComputeGradients(vector<double> &gradients,
                            vector<double> &loss_rates, double min_start_time_ms,
                            double max_finish_time_ms, int nopenmp_threads){
    int nlinks = data->inverse_links.size();
    assert(loss_rates.size() == nlinks);
    vector<double> gradients_threads[nopenmp_threads];
    for (int tt=0; tt<nopenmp_threads; tt++){
        gradients_threads[tt].resize(nlinks, 0.00);
    }
    //!TODO implement USE_CONDITIONAL
    assert(!USE_CONDITIONAL);
    #pragma omp parallel for num_threads(nopenmp_threads)
    for (int ff=0; ff<data->flows.size(); ff++){
        Flow* flow = data->flows[ff];
        if (DiscardFlow(flow, min_start_time_ms, max_finish_time_ms)) continue;
        vector<Path*>* flow_paths = flow->GetPaths(max_finish_time_ms);
        int npaths = flow_paths->size();
        static_assert(!CONSIDER_REVERSE_PATH);
        PII weight = flow->LabelWeightsFunc(max_finish_time_ms);
        if (weight.first == 0 and weight.second == 0) continue;
        //!TODO : flow_prob1 is redundant
        long double flow_prob1 = 0.0;
        long double flow_prob = flow->GetCachedIntermediateValue();
        int thread_num = omp_get_thread_num();
        for (Path* path: *flow_paths){
            double path_fail_rate = PathFailRate(flow->first_link_id, flow->last_link_id, path, loss_rates);
            flow_prob1 += FlowProbabilityForPath(weight.first, weight.second, path_fail_rate)/npaths;
            long double gradient_common = PathGradientNumerator(weight.first, weight.second, path_fail_rate);
            gradient_common *= (1.0 - path_fail_rate);
            gradient_common /= flow_prob;
            gradient_common /= npaths;
            gradients_threads[thread_num][flow->first_link_id] += gradient_common/
                                             (1.0-loss_rates[flow->first_link_id]);
            gradients_threads[thread_num][flow->last_link_id] += gradient_common/
                                             (1.0-loss_rates[flow->last_link_id]);
            for (int link_id: *path){
                gradients_threads[thread_num][link_id] += gradient_common/(1.0-loss_rates[link_id]);
            }
        }
        assert (abs(flow_prob - flow_prob1) < 1.0e-10);
    }
    gradients.resize(nlinks, 0.0);
    #pragma omp parallel for num_threads(nopenmp_threads)
    for (int link_id=0; link_id<nlinks; link_id++){
        //!TODO: Try changing 2d array from (threads X links) to (links X threads)
        for(int t=0; t<nopenmp_threads; t++){
            gradients[link_id] += gradients_threads[t][link_id];
        }
    }
}


