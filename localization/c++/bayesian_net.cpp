#include "bayesian_net.h"
#include <assert.h>
#include <algorithm>
#include <queue>
#include <chrono>
#include <cmath>
#include <iostream>

using namespace std;

//!TODO: Change to smart pointers for easier GC
//!TODO: Add result vector to argument list in compute likelihood function
Hypothesis* BayesianNet::LocalizeFailures(LogFileData* data, double min_start_time_ms,
                                double max_finish_time_ms, int nopenmp_threads){
    data_cache = data;
    flows_by_link_cache = data_cache->GetFlowsByLink(max_finish_time_ms);
    assert (flows_by_link_cache != NULL);
    unordered_map<Hypothesis*, double> all_hypothesis;

    Hypothesis* no_failure_hypothesis = new Hypothesis();
    all_hypothesis[no_failure_hypothesis] = 0.0;
    vector<Hypothesis*> prev_hypothesis_space; 
    //Initial list of candidates is all links
    vector<Link> candidates = data->inverse_links;
    prev_hypothesis_space.push_back(no_failure_hypothesis);
    int nfails = 1;
    bool repeat_nfails_1 = true;
    auto start_search_time = chrono::high_resolution_clock::now();
    while (nfails <= MAX_FAILS){
        auto start_stage_time = chrono::high_resolution_clock::now();
        vector<Hypothesis*> hypothesis_space;
        vector<pair<Hypothesis*, double> > base_hypothesis_likelihood;
        for(Hypothesis* h: prev_hypothesis_space){
            assert (h != NULL);
            for (Link &link: candidates){
                if (h->find(link) == h->end()){
                    h->insert(link);
                    if (find_if(hypothesis_space.begin(), hypothesis_space.end(),
                                        HypothesisPointerCompare(h)) == hypothesis_space.end()){
                        Hypothesis *hnew = new Hypothesis(*h);
                        hypothesis_space.push_back(hnew);
                        base_hypothesis_likelihood.push_back(make_pair(h, all_hypothesis[h]));
                        //base_hypothesis_likelihood.push_back(make_pair(no_failure_hypothesis, 0.0));
                    }
                    h->erase(link);
                }
            }
        }
        vector<pair<double, Hypothesis*> > result;
        ComputeLogLikelihood(hypothesis_space, base_hypothesis_likelihood, result,
                             min_start_time_ms, max_finish_time_ms, nopenmp_threads);
        sort(result.begin(), result.end(), greater<pair<double, Hypothesis*> >());
        if (nfails == 1){
            //update candidates
            candidates.clear();
            for (int i=0; i<min((int)result.size(), NUM_CANDIDATES); i++){
                Hypothesis* h = result[i].second;
                assert (h->size() == 1);
                Link link = *(h->begin());
                candidates.push_back(link);
            }
        }
        if (VERBOSE){
            cout << "Finished hypothesis search across "<<hypothesis_space.size()<<" Hypothesis for "
                 << nfails<<" failures in "<<chrono::duration_cast<chrono::milliseconds>(
                 chrono::high_resolution_clock::now() - start_stage_time).count()*1.0e-3
                 << " seconds" << endl;
                 //<<" function1_time_sec " << function1_time_sec<<endl;
            function1_time_sec = 0.0;
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
        Hypothesis failed_links;
        data_cache->GetFailedLinksSet(failed_links);
        double likelihood_correct_hypothesis = ComputeLogLikelihood(&failed_links,
                                                        no_failure_hypothesis, 0.0,
                                                        min_start_time_ms, max_finish_time_ms);
        cout << "Correct Hypothesis " << failed_links << " likelihood " << likelihood_correct_hypothesis << endl;

    }

    Hypothesis* failed_links = new Hypothesis();
    vector<pair<double, Hypothesis*> > likelihood_hypothesis;
    for (auto& it: all_hypothesis){
        likelihood_hypothesis.push_back(make_pair(it.second, it.first));
    }
    sort(likelihood_hypothesis.begin(), likelihood_hypothesis.end(),
                                greater<pair<double, Hypothesis*> >());
    double highest_likelihood = likelihood_hypothesis[0].first;
    for (int i=min((int)likelihood_hypothesis.size(), N_MAX_K_LIKELIHOODS)-1; i>=0; i--){
        double likelihood = likelihood_hypothesis[i].first;
        Hypothesis *hypothesis = likelihood_hypothesis[i].second;
        if (highest_likelihood - likelihood <= 1.0e-3){
            failed_links->insert(hypothesis->begin(), hypothesis->end());
        }
        if (VERBOSE){
            cout << "Likely candidate "<<*hypothesis<<" "<<likelihood << endl;
        }
    }
    if (VERBOSE){
        cout << endl << "Searched hypothesis space in "<<chrono::duration_cast<chrono::milliseconds>(
             chrono::high_resolution_clock::now() - start_search_time).count()*1.0e-3 << " seconds"<<endl;
    }
    return failed_links;
}


void BayesianNet::ComputeLogLikelihood(vector<Hypothesis*> &hypothesis_space,
                 vector<pair<Hypothesis*, double> > &base_hypothesis_likelihood,
                 vector<pair<double, Hypothesis*> > &result,
                 double min_start_time_ms, double max_finish_time_ms,
                 int nopenmp_threads){
    result.resize(hypothesis_space.size());
    #pragma omp parallel for num_threads(nopenmp_threads)
    for(int i=0; i<hypothesis_space.size(); i++){
        Hypothesis* h = hypothesis_space[i];
        auto base = base_hypothesis_likelihood[i];
        result[i] = make_pair(ComputeLogLikelihood(h, base.first, base.second,
                                        min_start_time_ms, max_finish_time_ms), h);
    }
}

inline double BayesianNet::BnfWeighted(int naffected, int npaths,
                                       int naffected_r, int npaths_r,
                                       double weight_good, double weight_bad){
    // e2e: end-to-end
    int e2e_paths = npaths * npaths_r;
    int e2e_correct_paths = (npaths - naffected) * (npaths_r - naffected_r);
    int e2e_failed_paths = e2e_paths - e2e_correct_paths;
    double a = ((double)e2e_failed_paths)/e2e_paths;
    return log((1.0 - a) + a * pow((1.0 - p1)/p2, weight_bad) * pow(p1/(1.0-p2), weight_good));
}

double BayesianNet::ComputeLogLikelihood(Hypothesis* hypothesis,
                    Hypothesis* base_hypothesis, double base_likelihood,
                    double min_start_time_ms, double max_finish_time_ms){
    double log_likelihood = 0.0;
    unordered_set<int> relevant_flows;
    for (Link link: *hypothesis){
        if (base_hypothesis->find(link) == base_hypothesis->end()){
            vector<int> &link_flows = (*flows_by_link_cache)[link];
            for(int f: link_flows){
                Flow* flow = data_cache->flows[f];
                //!TODO: Add option for active_flows_only
                if (flow->start_time_ms >= min_start_time_ms){
                    relevant_flows.insert(f);
                }
            }
        }
    }
    //auto start_compute_time = chrono::high_resolution_clock::now();
    for (int ff: relevant_flows){
        Flow *flow = data_cache->flows[ff];
        vector<Path*>* flow_paths = flow->GetPaths(max_finish_time_ms);
        PII weight = flow->LabelWeightsFunc(max_finish_time_ms);
        if (weight.first == 0 and weight.second == 0) continue;
        assert (flow_paths != NULL);
        int npaths = flow_paths->size(), naffected=0, naffected_base=0;
        //!TODO: Performance todo: check if dereferencing flow_paths ptr is okay in the for loop
        for (Path* path: *flow_paths){
            bool path_bad = false, path_bad_base = false;
            for (int i=1; i<path->size(); i++){
                Link link = make_pair(path->at(i-1), path->at(i));
                path_bad = (path_bad or (hypothesis->find(link) != hypothesis->end()));
                path_bad_base = (path_bad_base or 
                                 (base_hypothesis->find(link) != base_hypothesis->end()));
            }
            naffected += path_bad;
            naffected_base += path_bad_base;
        }
        vector<Path*>* flow_reverse_paths = flow->GetReversePaths(max_finish_time_ms);
        int npaths_r = flow_reverse_paths->size(), naffected_r = 0, naffected_base_r = 0;
        //!TODO: implement REVERSE_PATH case
        log_likelihood += BnfWeighted(naffected, npaths, naffected_r, npaths_r,
                                      weight.first, weight.second);
        if (naffected_base > 0 or naffected_base_r > 0){
            log_likelihood -= BnfWeighted(naffected_base, npaths,
                              naffected_base_r, npaths_r, weight.first, weight.second);
        }
    }
    //function1_time_sec += chrono::duration_cast<chrono::nanoseconds>(
    //             chrono::high_resolution_clock::now() - start_compute_time).count();
    log_likelihood = max(-1.0e9, log_likelihood);
    //cout << *hypothesis << " " << log_likelihood + hypothesis->size() * PRIOR << endl;
    return base_likelihood + log_likelihood + (hypothesis->size() - base_hypothesis->size()) * PRIOR;
}