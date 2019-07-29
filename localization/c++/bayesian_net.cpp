#include "bayesian_net.h"
#include <assert.h>
#include <algorithm>
#include <queue>
#include <chrono>
#include <cmath>
#include <iostream>

using namespace std;

//!TODO: Change to smart pointers for easier GC
//!TODO: Change to smart pointers for easier GC
//!TODO: Change to smart pointers for easier GC
//!TODO: Change to smart pointers for easier GC
//!TODO: Change to smart pointers for easier GC
Hypothesis* BayesianNet::LocalizeFailures(LogFileData* data, double start_time_ms, double max_finish_time_ms){
    data_cache = data;
    flows_by_link_cache = data_cache->GetFlowsByLink(max_finish_time_ms);
    assert (flows_by_link_cache != NULL);
    /*
    priority_queue<pair<double, Hypothesis*>,
                   vector<pair<double, Hypothesis*> >,
                   std::greater<double> > max_k_likelihoods;
    */
    Hypothesis* no_failure_hypothesis = new Hypothesis();
    //max_k_likelihoods.push(make_pair(0.0, no_failure_hypothesis));
    vector<Hypothesis*> prev_hypothesis_space; 
    //Initial list of candidates is all links
    vector<Link> candidates = data->inverse_links;
    prev_hypothesis_space.push_back(no_failure_hypothesis);
    int nfails = 1;
    bool repeat_nfails_1 = true;
    while (nfails <= MAX_FAILS){
        auto start_search_time = chrono::high_resolution_clock::now();
        vector<Hypothesis*> hypothesis_space;
        for(Hypothesis* h: prev_hypothesis_space){
            assert (h != NULL);
            for (Link &link: candidates){
                if (h->find(link) == h->end()){
                    h->insert(link);
                    if (find_if(hypothesis_space.begin(), hypothesis_space.end(), HypothesisPointerCompare(h)) == hypothesis_space.end()){
                        Hypothesis *hnew = new Hypothesis();
                        *hnew = *h;
                        hypothesis_space.push_back(hnew);
                    }
                    h->erase(link);
                }
            }
        }
        /*
        for (Hypothesis *h : hypothesis_space){
            cout<< *h << " ";
        }
        cout<< endl;
        */
        vector<pair<double, Hypothesis*> > *result = ComputeLogLikelihood(hypothesis_space, start_time_ms, max_finish_time_ms);
        sort(result->begin(), result->end(), greater<pair<double, Hypothesis*> >());
        if (nfails == 1){
            //update candidates
            candidates.clear();
            for (int i=0; i<min((int)result->size(), NUM_CANDIDATES); i++){
                Hypothesis* h = result->at(i).second;
                assert (h->size() == 1);
                Link link = *(h->begin());
                candidates.push_back(link);
            }
        }
        if (VERBOSE){
            cout << "Finished hypothesis search across "<<hypothesis_space.size()<<" Hypothesis for "
                 <<nfails<<" failures in "<<chrono::duration_cast<chrono::milliseconds>(
                 chrono::high_resolution_clock::now() - start_search_time).count()/1000.0 <<" seconds" << endl;
        }
        nfails++;
        prev_hypothesis_space.clear();
        for (int i=0; i<min((int)result->size(), NUM_TOP_HYPOTHESIS_AT_EACH_STAGE); i++){
            prev_hypothesis_space.push_back(result->at(i).second);
        }
    }
    Hypothesis* failed_links = new Hypothesis();
    /*
    while (max_k_likelihoods.size() > 0){
        pair<double, Hypothesis*> elt = max_k_likelihoods
        if (highest_likelihood - elt.first <= 1.0e-3){
            failed_links->insert(elt.second->begin(), elt.second->end());
        }
        if (VERBOSE){
            cout << "Likely candidate "<<elt.second<<" "<<elt.first << endl;
        }
    }
    */
    return failed_links;
}


vector<pair<double, Hypothesis*> >* BayesianNet::ComputeLogLikelihood(vector<Hypothesis*> &hypothesis_space, double min_start_time_ms, double max_finish_time_ms){
    vector<pair<double, Hypothesis*> > *result = new vector<pair<double, Hypothesis*> >();
    for (Hypothesis* h: hypothesis_space){
        result->push_back(make_pair(ComputeLogLikelihood(h, min_start_time_ms, max_finish_time_ms), h));
    }
    return result;
}

double BayesianNet::BnfWeighted(int naffected, int npaths, int naffected_r, int npaths_r, double weight_good, double weight_bad){
    // e2e: end-to-end
    int e2e_paths = npaths * npaths_r;
    int e2e_correct_paths = (npaths - naffected) * (npaths_r - naffected_r);
    int e2e_failed_paths = e2e_paths - e2e_correct_paths;
    double a = ((double)e2e_failed_paths)/e2e_paths;
    double val = (1.0 - a) + a * pow((1.0 - p1)/p2, weight_bad) * pow(p1/(1.0-p2), weight_good);
    return log(val);
}

double BayesianNet::ComputeLogLikelihood(Hypothesis* hypothesis, double min_start_time_ms, double max_finish_time_ms){
    double log_likelihood = 0.0;
    unordered_set<int> relevant_flows;
    for (Link link: *hypothesis){
        vector<int> &link_flows = (*flows_by_link_cache)[link];
        for(int f: link_flows){
            Flow* flow = data_cache->flows[f];
            //!TODO: Add option for active_flows_only
            if (flow->start_time_ms >= min_start_time_ms){
                relevant_flows.insert(f);
            }
        }
    }
    for (int ff: relevant_flows){
        Flow *flow = data_cache->flows[ff];
        vector<Path*>* flow_paths = flow->GetPaths(max_finish_time_ms);
        PII weight = flow->LabelWeightsFunc(max_finish_time_ms);
        if (weight.first == 0 and weight.second == 0) continue;
        int npaths = flow_paths->size(), naffected=0;
        //!TODO: Performance todo: check if dereferencing flow_paths ptr is okay in the for loop
        for (Path* path: *flow_paths){
            for (int i=1; i<path->size(); i++){
                Link link = make_pair(path->at(i-1), path->at(i));
                if (hypothesis->find(link) != hypothesis->end()){
                    naffected++;
                    break;
                }
            }
        }
        vector<Path*>* flow_reverse_paths = flow->GetReversePaths(max_finish_time_ms);
        int npaths_r = flow_reverse_paths->size(), naffected_r = 0.0;
        //!TODO: implement REVERSE_PATH case
        log_likelihood += BnfWeighted(naffected, npaths, naffected_r, npaths_r, weight.first, weight.second);
        /* if (log_likelihood < -1000.0){
            cout << npaths << " "<<naffected << " " << weight << endl;
        } */
    }
    log_likelihood = max(-1.0e9, log_likelihood);
    //cout << *hypothesis << " " << log_likelihood + hypothesis->size() * PRIOR << endl;
    return log_likelihood + hypothesis->size() * PRIOR;
}
