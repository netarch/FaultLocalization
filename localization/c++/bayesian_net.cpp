#include "bayesian_net.h"
#include <assert.h>
#include <algorithm>
#include <queue>
#include <chrono>
#include <cmath>
#include <iostream>
#include <omp.h>
#include <sparsehash/dense_hash_set> 
using google::dense_hash_set;
using namespace std;

//!TODO: Change to smart pointers for easier GC
//!TODO: Add code for REVERSE_PATH
void BayesianNet::LocalizeFailures(LogFileData* data, double min_start_time_ms,
                                double max_finish_time_ms, Hypothesis& localized_links,
                                int nopenmp_threads){
    if (VERBOSE) cout << "Num flows "<<data->flows.size()<<endl;
    data_cache = data;
    auto start_bin_time = chrono::high_resolution_clock::now();
    flows_by_link_id_cache = data_cache->GetFlowsByLinkId(max_finish_time_ms, nopenmp_threads);
    if (VERBOSE){
        cout << "Finished binning flows by links in "<<chrono::duration_cast<chrono::milliseconds>(
             chrono::high_resolution_clock::now() - start_bin_time).count()*1.0e-3
             << " seconds" << endl;
    }
    assert (flows_by_link_id_cache != NULL);
    unordered_map<Hypothesis*, double> all_hypothesis;

    Hypothesis* no_failure_hypothesis = new Hypothesis();
    all_hypothesis[no_failure_hypothesis] = 0.0;
    vector<Hypothesis*> prev_hypothesis_space; 
    //Initial list of candidates is all links
    //!TODO
    vector<int> candidates;
    for (int link_id=0; link_id<data_cache->inverse_links.size(); link_id++){
        candidates.push_back(link_id);
    }
    prev_hypothesis_space.push_back(no_failure_hypothesis);
    int nfails = 1;
    bool repeat_nfails_1 = true;
    auto start_search_time = chrono::high_resolution_clock::now();
    while (nfails <= MAX_FAILS){
        auto start_stage_time = chrono::high_resolution_clock::now();
        vector<pair<double, Hypothesis*> > result;
        if (nfails == 1 and repeat_nfails_1) {
            ComputeSingleLinkLogLikelihood(result, min_start_time_ms,
                                       max_finish_time_ms, nopenmp_threads);
        }
        else{
            // Inefficient if |hypothesis_space| is large, say > 1000
            vector<Hypothesis*> hypothesis_space;
            vector<pair<Hypothesis*, double> > base_hypothesis_likelihood;
            //auto start_init_candidates = chrono::high_resolution_clock::now();
            for(Hypothesis* h: prev_hypothesis_space){
                assert (h != NULL);
                for (int link_id: candidates){
                    if (h->find(link_id) == h->end()){
                        h->insert(link_id);
                        if (find_if(hypothesis_space.begin(), hypothesis_space.end(),
                                            HypothesisPointerCompare(h)) == hypothesis_space.end()){
                            Hypothesis *hnew = new Hypothesis(*h);
                            hypothesis_space.push_back(hnew);
                            base_hypothesis_likelihood.push_back(make_pair(h, all_hypothesis[h]));
                            //base_hypothesis_likelihood.push_back(make_pair(no_failure_hypothesis, 0.0));
                        }
                        h->erase(link_id);
                    }
                }
            }
            /*
            if (VERBOSE){
                cout << "Finished initializing candidates flows in "<<chrono::duration_cast<chrono::milliseconds>(
                     chrono::high_resolution_clock::now() - start_init_candidates).count()*1.0e-3
                     << " seconds" << endl;
            }
            */
            ComputeLogLikelihood(hypothesis_space, base_hypothesis_likelihood, result,
                             min_start_time_ms, max_finish_time_ms, nopenmp_threads);
        }
        
        sort(result.begin(), result.end(), greater<pair<double, Hypothesis*> >());
        if (nfails == 1){
            //update candidates
            candidates.clear();
            for (int i=0; i<min((int)result.size(), NUM_CANDIDATES); i++){
                Hypothesis* h = result[i].second;
                assert (h->size() == 1);
                int link_id = *(h->begin());
                candidates.push_back(link_id);
            }
        }
        if (VERBOSE){
            cout << "Finished hypothesis search across "<<result.size()<<" Hypothesis for "
                 << nfails<<" failures in "<<chrono::duration_cast<chrono::milliseconds>(
                 chrono::high_resolution_clock::now() - start_stage_time).count()*1.0e-3
                 << " seconds" << endl;
            //for (int t = 0; t < nopenmp_threads; t++){
            //    cout <<" function1_time_sec " << function1_time_sec[t] * 1.0e-9 <<endl;
            //    function1_time_sec[t] = 0.0;
            //}
        }
        nfails++;
        prev_hypothesis_space.clear();
        for (int i=0; i<min((int)result.size(), NUM_TOP_HYPOTHESIS_AT_EACH_STAGE); i++){
            prev_hypothesis_space.push_back(result[i].second);
        }
        for (auto& it: result){
            all_hypothesis[it.second] = it.first;
        }
    }
    if (VERBOSE) {
        Hypothesis correct_hypothesis;
        data_cache->GetFailedLinkIds(correct_hypothesis);
        double likelihood_correct_hypothesis = ComputeLogLikelihood(&correct_hypothesis,
                                                        no_failure_hypothesis, 0.0,
                                                        min_start_time_ms, max_finish_time_ms);
        cout << "Correct Hypothesis " << data_cache->IdsToLinks(correct_hypothesis) << " likelihood " << likelihood_correct_hypothesis << endl;

    }

    vector<pair<double, Hypothesis*> > likelihood_hypothesis;
    for (auto& it: all_hypothesis){
        likelihood_hypothesis.push_back(make_pair(it.second, it.first));
    }
    sort(likelihood_hypothesis.begin(), likelihood_hypothesis.end(),
                                greater<pair<double, Hypothesis*> >());
    localized_links.clear();
    double highest_likelihood = likelihood_hypothesis[0].first;
    for (int i=min((int)likelihood_hypothesis.size(), N_MAX_K_LIKELIHOODS)-1; i>=0; i--){
        double likelihood = likelihood_hypothesis[i].first;
        Hypothesis *hypothesis = likelihood_hypothesis[i].second;
        if (highest_likelihood - likelihood <= 1.0e-3){
            localized_links.insert(hypothesis->begin(), hypothesis->end());
        }
        if (VERBOSE){
            cout << "Likely candidate "<<data_cache->IdsToLinks(*hypothesis)<<" "<<likelihood << endl;
        }
    }
    if (VERBOSE){
        cout << endl << "Searched hypothesis space in "<<chrono::duration_cast<chrono::milliseconds>(
             chrono::high_resolution_clock::now() - start_search_time).count()*1.0e-3 << " seconds"<<endl;
    }
}

void BayesianNet::ComputeSingleLinkLogLikelihood(vector<pair<double, Hypothesis*> > &result,
                                        double min_start_time_ms, double max_finish_time_ms,
                                        int nopenmp_threads){
    int nlinks = data_cache->inverse_links.size();
    cout << "nlinks "<<nlinks<< " nopenmp_threads "<<nopenmp_threads<<endl;
    vector<double> likelihoods[nopenmp_threads];
    #pragma omp parallel for num_threads(nopenmp_threads)
    for(int t=0; t<nopenmp_threads; t++){
        likelihoods[t].resize(nlinks);
        std::fill_n(likelihoods[t].begin(), nlinks, 0.0);
    }
    //double map_update_time[nopenmp_threads] = {0.0};
    #pragma omp parallel for num_threads(nopenmp_threads)
    for (int ii=0; ii<data_cache->flows.size(); ii++){
        Flow* flow = data_cache->flows[ii];
        int thread_num = omp_get_thread_num();
        vector<Path*>* flow_paths = flow->GetPaths(max_finish_time_ms);
        int npaths = flow_paths->size(), naffected = 1;
        int npaths_r = 1, naffected_r = 0;
        PII weight = flow->LabelWeightsFunc(max_finish_time_ms);
        if (weight.first == 0 and weight.second == 0) continue;
        //!TODO: implement REVERSE_PATH case
        if (CONSIDER_REVERSE_PATH) {assert (false);}
        double log_likelihood;
        if (USE_CONDITIONAL) {
            log_likelihood = BnfWeightedConditional(naffected, npaths, naffected_r, npaths_r,
                                          weight.first, weight.second);
        }
        else {
            log_likelihood = BnfWeighted(naffected, npaths, naffected_r, npaths_r,
                                          weight.first, weight.second);
        }
        //auto start_map_update_time = chrono::high_resolution_clock::now();
        for (Path* path: *flow_paths){
            for (int link_id: *path){
                likelihoods[thread_num][link_id] += log_likelihood;
            }
        }
        if (MEMOIZE_PATHS){
            // For the first and last links that are common to all paths
            naffected = npaths;
            if (USE_CONDITIONAL) {
                log_likelihood = BnfWeightedConditional(naffected, npaths, naffected_r, npaths_r,
                                              weight.first, weight.second);
            }
            else {
                log_likelihood = BnfWeighted(naffected, npaths, naffected_r, npaths_r,
                                              weight.first, weight.second);
            }
            likelihoods[thread_num][flow->first_link_id] += log_likelihood;
            likelihoods[thread_num][flow->last_link_id] += log_likelihood;
        }
        //map_update_time[thread_num] += chrono::duration_cast<chrono::microseconds>(
        //        chrono::high_resolution_clock::now() - start_map_update_time).count()*1.0e-6;
    }
    double final_likelihoods[nlinks];
    fill(final_likelihoods, final_likelihoods + nlinks, 0.0);
    #pragma omp parallel for num_threads(nopenmp_threads)
    for (int link_id=0; link_id<nlinks; link_id++){
        //!TODO: Try changing likelihoods 2d array from (threads X links) to (links X threads)
        for(int t=0; t<nopenmp_threads; t++){
            final_likelihoods[link_id] += likelihoods[t][link_id];
        }
    }
    auto start_init_candidates = chrono::high_resolution_clock::now();
    result.resize(nlinks);
    for(int link_id=0; link_id<nlinks; link_id++){
        Hypothesis* h = new Hypothesis();
        h->insert(link_id);
        result[link_id] = make_pair(final_likelihoods[link_id], h);
    }
    if (VERBOSE){
        cout << "Finished initializing candidates flows in "<<chrono::duration_cast<chrono::milliseconds>(
             chrono::high_resolution_clock::now() - start_init_candidates).count()*1.0e-3
             << " seconds" << endl;
    }
}

void BayesianNet::ComputeLogLikelihood(vector<Hypothesis*> &hypothesis_space,
                 vector<pair<Hypothesis*, double> > &base_hypothesis_likelihood,
                 vector<pair<double, Hypothesis*> > &result,
                 double min_start_time_ms, double max_finish_time_ms,
                 int nopenmp_threads){
    result.resize(hypothesis_space.size());
    //#pragma omp parallel for num_threads(nopenmp_threads) schedule(dynamic, 1)
    vector<int> relevant_flows[hypothesis_space.size()];
    #pragma omp parallel for num_threads(nopenmp_threads)
    for(int i=0; i<hypothesis_space.size(); i++){
        Hypothesis* h = hypothesis_space[i];
        auto& base = base_hypothesis_likelihood[i];
        GetRelevantFlows(h, base.first, min_start_time_ms, max_finish_time_ms, relevant_flows[i]);
    }
    for(int i=0; i<hypothesis_space.size(); i++){
        Hypothesis* h = hypothesis_space[i];
        auto& base = base_hypothesis_likelihood[i];
        result[i] = pair<double, Hypothesis*>(ComputeLogLikelihood(h, base.first, base.second,
                    min_start_time_ms, max_finish_time_ms, relevant_flows[i]), h);
    }
}

inline double BayesianNet::BnfWeightedConditional(int naffected, int npaths,
                                       int naffected_r, int npaths_r,
                                       double weight_good, double weight_bad){
    // e2e: end-to-end
    int e2e_paths = npaths * npaths_r;
    int e2e_failed_paths = e2e_paths - (npaths - naffected) * (npaths_r - naffected_r);
    double a = ((double)e2e_failed_paths)/e2e_paths;
    double val1 = (1.0 - a) + a * pow(((1.0 - p1)/p2), weight_bad) * pow((p1/(1.0-p2)), weight_good);
    double total_weight = weight_good + weight_bad;
    double val2 = (1.0 - a) + a * (1 - pow(p1, total_weight))/(1.0 - pow((1.0 - p2), total_weight));
    return log(val1/val2);
}

inline double BayesianNet::BnfWeighted(int naffected, int npaths,
                                       int naffected_r, int npaths_r,
                                       double weight_good, double weight_bad){
    // e2e: end-to-end
    int e2e_paths = npaths * npaths_r;
    int e2e_failed_paths = e2e_paths - (npaths - naffected) * (npaths_r - naffected_r);
    double a = ((double)e2e_failed_paths)/e2e_paths;
    return log((1.0 - a) + a * pow((1.0 - p1)/p2, weight_bad) * pow(p1/(1.0-p2), weight_good));
}



void BayesianNet::GetRelevantFlows(Hypothesis* hypothesis, Hypothesis* base_hypothesis,
                                   double min_start_time_ms, double max_finish_time_ms,
                                   vector<int>& relevant_flows){
    auto start_init_time = chrono::high_resolution_clock::now();
    dense_hash_set<int, hash<int> > relevant_flows_set;
    relevant_flows_set.set_empty_key(-1);
    for (int link_id: *hypothesis){
        if (base_hypothesis->find(link_id) == base_hypothesis->end()){
            vector<int> &link_id_flows = (*flows_by_link_id_cache)[link_id];
            for(int f: link_id_flows){
                Flow* flow = data_cache->flows[f];
                //!TODO: Add option for active_flows_only
                if (flow->start_time_ms >= min_start_time_ms and
                    (!USE_CONDITIONAL or flow->TracerouteFlow(max_finish_time_ms))){
                    relevant_flows_set.insert(f);
                }
            }
        }
    }
    relevant_flows.insert(relevant_flows.begin(), relevant_flows_set.begin(), relevant_flows_set.end());
}

double BayesianNet::ComputeLogLikelihood(Hypothesis* hypothesis, Hypothesis* base_hypothesis,
                    double base_likelihood, double min_start_time_ms,
                    double max_finish_time_ms){
    vector<int> relevant_flows;
    GetRelevantFlows(hypothesis, base_hypothesis, min_start_time_ms, max_finish_time_ms, relevant_flows);
    return ComputeLogLikelihood(hypothesis, base_hypothesis, base_likelihood, min_start_time_ms,
                                max_finish_time_ms, relevant_flows);
}

double BayesianNet::ComputeLogLikelihood(Hypothesis* hypothesis, Hypothesis* base_hypothesis,
                    double base_likelihood, double min_start_time_ms,
                    double max_finish_time_ms, vector<int> &relevant_flows){
    auto start_compute_time = chrono::high_resolution_clock::now();
    //for (int ff: relevant_flows){
    //    Flow *flow = data_cache->flows[ff];
    atomic<double> log_likelihood_atomic = 0.0;
    #pragma omp parallel for num_threads(40)
    for (int ii=0; ii<relevant_flows.size(); ii++){
        Flow *flow = data_cache->flows[relevant_flows[ii]];
        double log_likelihood = 0.0;
        vector<Path*>* flow_paths = flow->GetPaths(max_finish_time_ms);
        PII weight = flow->LabelWeightsFunc(max_finish_time_ms);
        if (weight.first == 0 and weight.second == 0) continue;
        assert (flow_paths != NULL);
        int npaths = flow_paths->size(), naffected=0, naffected_base=0;
        // If common links of all paths are in hypothesis, then all paths are affected
        if (MEMOIZE_PATHS and ((hypothesis->find(flow->first_link_id) != hypothesis->end())
                            or (hypothesis->find(flow->last_link_id) != hypothesis->end()))){
            naffected = npaths;
        }
        else{
            for (Path* path: *flow_paths){
                bool path_bad = false;
                for (int link_id: *path){
                    path_bad = (path_bad or (hypothesis->find(link_id) != hypothesis->end()));
                }
                naffected += path_bad;
            }
        }
        if (MEMOIZE_PATHS and (base_hypothesis->find(flow->first_link_id) != base_hypothesis->end())
                           or (base_hypothesis->find(flow->last_link_id) != base_hypothesis->end())){
            naffected_base = npaths;
        }
        else{
            for (Path* path: *flow_paths){
                bool path_bad_base = false;
                for (int link_id: *path){
                    path_bad_base = (path_bad_base or 
                            (base_hypothesis->find(link_id) != base_hypothesis->end()));
                }
                naffected_base += path_bad_base;
            }
        }
        int npaths_r = 1;
        int naffected_r = 0, naffected_base_r = 0;
        if (CONSIDER_REVERSE_PATH){
            vector<Path*>* flow_reverse_paths = flow->GetReversePaths(max_finish_time_ms);
            npaths_r = flow_reverse_paths->size();
            //!TODO: computed naffected_r, naffected_base_r
        }
        //!TODO: implement REVERSE_PATH case
        if (USE_CONDITIONAL) {
            log_likelihood += BnfWeightedConditional(naffected, npaths, naffected_r, npaths_r,
                                                     weight.first, weight.second);
            if (naffected_base > 0 or naffected_base_r > 0){
                log_likelihood -= BnfWeightedConditional(naffected_base, npaths, naffected_base_r,
                                                         npaths_r, weight.first, weight.second);
            }
        }
        else {
            log_likelihood += BnfWeighted(naffected, npaths, naffected_r, npaths_r,
                                          weight.first, weight.second);
            if (naffected_base > 0 or naffected_base_r > 0){
                log_likelihood -= BnfWeighted(naffected_base, npaths, naffected_base_r,
                                              npaths_r, weight.first, weight.second);
            }
        }
        atomic_add_double(log_likelihood_atomic, log_likelihood);
    }
    double compute_time_seconds = chrono::duration_cast<chrono::nanoseconds>(
                 chrono::high_resolution_clock::now() - start_compute_time).count() * 1.0e-9;
    //if (compute_time_seconds > 0.01) cout << "compute_time_seconds " << compute_time_seconds << endl;
    //function1_time_sec[omp_get_thread_num()] += chrono::duration_cast<chrono::nanoseconds>(
    //             chrono::high_resolution_clock::now() - start_compute_time).count();
    //log_likelihood = max(-1.0e9, log_likelihood);
    double log_likelihood = max(-1.0e9, (double)log_likelihood_atomic);
    //cout << *hypothesis << " " << log_likelihood + hypothesis->size() * PRIOR << endl;
    return base_likelihood + log_likelihood + (hypothesis->size() - base_hypothesis->size()) * PRIOR;
}

inline double atomic_add_double(atomic<double> &d, double val){
    double old_val = d.load(memory_order_consume);
    double new_val = old_val + val;
    while (!d.compare_exchange_weak(old_val, new_val, memory_order_release, memory_order_consume)){
        new_val = d + val;
    }
    return new_val;
}
