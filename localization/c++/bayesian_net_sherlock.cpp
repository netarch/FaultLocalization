#include "bayesian_net_sherlock.h"
#include "utils.h"
#include <limits>
#include <assert.h>
#include <numeric>
#include <algorithm>
#include <stack>
#include <queue>
#include <chrono>
#include <cmath>
#include <iostream>
#include <omp.h>
#include <tuple>
#include <sparsehash/dense_hash_set> 
using google::dense_hash_set;
using namespace std;

Sherlock* Sherlock::CreateObject(){
    Sherlock* ret = new Sherlock();
    vector<double> param {p1, p2, PRIOR};
    ret->SetParams(param);
    return ret;
}



void Sherlock::ExploreTopNode(double min_start_time_ms, double max_finish_time_ms,
                              unordered_map<Hypothesis*, double> &all_hypothesis, int nopenmp_threads, stack<PHS> &hstack){
        auto[base_hypothesis, base_scores] = hstack.top();
        hstack.pop();
        double base_likelihood = all_hypothesis[base_hypothesis];
        auto max_it = std::max_element(base_hypothesis->begin(), base_hypothesis->end());
        int max_link_id = (max_it == base_hypothesis->end()? -1: *max_it);
        vector<pair<double, Hypothesis*> > result;
        for (int link_id = data->inverse_links.size()-1; link_id>max_link_id; link_id--){
            Hypothesis *new_hypothesis = new Hypothesis(*base_hypothesis);
            new_hypothesis->insert(link_id);
            double new_likelihood = base_likelihood + base_scores->at(link_id)
                                                    + ComputeLogPrior(new_hypothesis)
                                                    - ComputeLogPrior(base_hypothesis);
            if (new_hypothesis->size() < MAX_FAILS){
                vector<double> *new_scores = new vector<double>(*base_scores);
                UpdateScores(*new_scores, new_hypothesis, base_hypothesis, min_start_time_ms, max_finish_time_ms, nopenmp_threads);
                hstack.push(PHS(new_hypothesis, new_scores));
                ExploreTopNode(min_start_time_ms, max_finish_time_ms, all_hypothesis, nopenmp_threads, hstack);
                //ExploreTopNode();
                if (link_id % 1000 == 0) cout << "pushing to stack " << *new_hypothesis << endl;
            }
            result.push_back(make_pair(new_likelihood, new_hypothesis));
        }
        sort(result.begin(), result.end(), greater<pair<double, Hypothesis*> >());
        int nsave = (base_hypothesis->size() < MAX_FAILS-1? result.size(): min(10, (int)result.size()));
        //cout << nsave << " " << base_hypothesis->size() << endl;
        int curr = 0;
        while(curr < nsave){
            all_hypothesis[result[curr].second] = result[curr].first;
            curr++;
        }
        while (curr < result.size() and base_hypothesis->size() == MAX_FAILS-1){
            delete result[curr].second;
            curr++;
        }
        delete base_scores;
};



void Sherlock::SearchHypothesesFlock(double min_start_time_ms, double max_finish_time_ms,
                                    unordered_map<Hypothesis*, double> &all_hypothesis,
                                    int nopenmp_threads){
    Hypothesis* no_failure_hypothesis = new Hypothesis();
    all_hypothesis[no_failure_hypothesis] = 0.0;
    vector<double>* initial_likelihoods = new vector<double>();
    ComputeInitialLikelihoods(*initial_likelihoods, min_start_time_ms, max_finish_time_ms, nopenmp_threads);
   
    stack<PHS> hstack;
    hstack.push(PHS(no_failure_hypothesis, initial_likelihoods));

    int cnt = 0;
    auto start_search_time = chrono::high_resolution_clock::now();
    ExploreTopNode(min_start_time_ms, max_finish_time_ms, all_hypothesis, nopenmp_threads, hstack);

    /*
    while(!hstack.empty()){
        if constexpr (VERBOSE){
            if (cnt++ % 500 == 0){
                cout << MAX_FAILS << " Finished traversing "<<cnt<<" Hypotheses in "<< 
                        GetTimeSinceSeconds(start_search_time) << " seconds, " << "stack size " << hstack.size() << endl;
            }
        }
        auto[base_hypothesis, base_scores] = hstack.top();
        hstack.pop();
        ExploreNode (base_hypothesis
        double base_likelihood = all_hypothesis[base_hypothesis];
        auto max_it = std::max_element(base_hypothesis->begin(), base_hypothesis->end());
        int max_link_id = (max_it == base_hypothesis->end()? -1: *max_it);
        vector<pair<double, Hypothesis*> > result;
        for (int link_id = data->inverse_links.size()-1; link_id>max_link_id; link_id--){
            Hypothesis *new_hypothesis = new Hypothesis(*base_hypothesis);
            new_hypothesis->insert(link_id);
            double new_likelihood = base_likelihood + base_scores->at(link_id)
                                                    + ComputeLogPrior(new_hypothesis)
                                                    - ComputeLogPrior(base_hypothesis);
            if (new_hypothesis->size() < MAX_FAILS){
                vector<double> *new_scores = new vector<double>(*base_scores);
                UpdateScores(*new_scores, new_hypothesis, base_hypothesis, min_start_time_ms, max_finish_time_ms, nopenmp_threads);
                hstack.push(PHS(new_hypothesis, new_scores));
                //cout << "pushing to stack " << *new_hypothesis << endl;
            }
            result.push_back(make_pair(new_likelihood, new_hypothesis));
        }
        sort(result.begin(), result.end(), greater<pair<double, Hypothesis*> >());
        int nsave = (base_hypothesis->size() < MAX_FAILS-1? result.size(): min(10, (int)result.size()));
        //cout << nsave << " " << base_hypothesis->size() << endl;
        int curr = 0;
        while(curr < nsave){
            all_hypothesis[result[curr].second] = result[curr].first;
            curr++;
        }
        while (curr < result.size() and base_hypothesis->size() == MAX_FAILS-1){
            delete result[curr].second;
            curr++;
        }
        delete base_scores;
    }*/
}

void Sherlock::SearchHypotheses(double min_start_time_ms, double max_finish_time_ms,
                                    unordered_map<Hypothesis*, double> &all_hypothesis,
                                    int nopenmp_threads){
    Hypothesis* no_failure_hypothesis = new Hypothesis();
    all_hypothesis[no_failure_hypothesis] = 0.0;
    //Initial list of candidates is all links
    vector<int> candidates;
    for (int link_id=0; link_id<data->inverse_links.size(); link_id++){
        candidates.push_back(link_id);
    }
    vector<Hypothesis*> prev_hypothesis_space, next_hypothesis_space; 
    prev_hypothesis_space.push_back(no_failure_hypothesis);
    int nfails = 1;
    while (nfails <= MAX_FAILS) {
        auto start_stage_time = chrono::high_resolution_clock::now();
        next_hypothesis_space.clear();
        int hypothesis_analyzed = 0, flows_analyzed = 0;
        int ctr = 0;
        for(Hypothesis* h: prev_hypothesis_space){
            vector<Hypothesis*> hypothesis_space;
            vector<pair<Hypothesis*, double> > base_hypothesis_likelihood;
            assert (h != NULL);
            auto max_it = std::max_element(h->begin(), h->end());
            int max_link_id = (max_it == h->end()? -1: *max_it);
            for (int link_id: candidates){
                if (link_id > max_link_id){
                    h->insert(link_id);
                    Hypothesis *hnew = new Hypothesis(*h);
                    hypothesis_space.push_back(hnew);
                    base_hypothesis_likelihood.push_back(make_pair(h, all_hypothesis[h]));
                    //base_hypothesis_likelihood.push_back(make_pair(no_failure_hypothesis, 0.0));
                    h->erase(link_id);
                }
            }
            vector<pair<double, Hypothesis*> > result;
            ComputeLogLikelihood(hypothesis_space, base_hypothesis_likelihood, result,
                             min_start_time_ms, max_finish_time_ms, nopenmp_threads);
            sort(result.begin(), result.end(), greater<pair<double, Hypothesis*> >());
            hypothesis_analyzed += result.size();
            if (nfails < MAX_FAILS){
                for (int i=0; i<result.size(); i++){
                    next_hypothesis_space.push_back(result[i].second);
                }
            }
            int nsave = (nfails<MAX_FAILS? result.size(): min(10, (int)result.size()));
            int curr=0;
            while (curr < nsave){
                all_hypothesis[result[curr].second] = result[curr].first;
                curr++;
            }
            while (curr < result.size() and nfails == MAX_FAILS){
                delete result[curr].second;
                curr++;
            }
            if (++ctr % 250 == 0)
            cout << "Finished hypothesis search across "<<hypothesis_analyzed<<" Hypothesis for " << nfails
                 <<" failures in "<< GetTimeSinceSeconds(start_stage_time) << " seconds" << endl;
        }
        if constexpr (VERBOSE){
            cout << "Finished hypothesis search across "<<hypothesis_analyzed<<" Hypothesis for " << nfails
                 <<" failures in "<< GetTimeSinceSeconds(start_stage_time) << " seconds" << endl;
        }
        for (Hypothesis *h: prev_hypothesis_space){
            if (all_hypothesis.find(h) == all_hypothesis.end()) delete (h);
        }
        prev_hypothesis_space.clear();
        prev_hypothesis_space.insert(prev_hypothesis_space.begin(), next_hypothesis_space.begin(), next_hypothesis_space.end());
        nfails++;
    }
    cout << "Finished: " << all_hypothesis[no_failure_hypothesis] << " " << all_hypothesis.size() << endl;
}

void Sherlock::LocalizeFailures(double min_start_time_ms, double max_finish_time_ms,
                                   Hypothesis& localized_links, int nopenmp_threads){
    if constexpr (VERBOSE){
        cout << "Params: p1 " << 1.0 - p1 << " p2 " << p2 << " Prior " << PRIOR << " openmp threads " << nopenmp_threads << endl;
        cout << "start_time_ms " << min_start_time_ms << " " << max_finish_time_ms << endl;
    }

    assert(data != NULL);
    BinFlowsByLinkId(max_finish_time_ms, nopenmp_threads);

    auto start_time = timer_checkpoint = chrono::high_resolution_clock::now();
    if (!USE_CONDITIONAL){
        ComputeAndStoreIntermediateValues(nopenmp_threads, max_finish_time_ms);
    }
    if constexpr (VERBOSE){
        cout << "Finished computing intermediate values in "
             << GetTimeSinceSeconds(start_time) << " seconds" << endl;
    }
    if (VERBOSE and PRINT_SCORES){
        auto start_print_time = chrono::high_resolution_clock::now();
        PrintScores(min_start_time_ms, max_finish_time_ms, nopenmp_threads);
        cout << "Finished printing scores in " << GetTimeSinceSeconds(start_print_time)
             << " seconds" << endl;
    }

    unordered_map<Hypothesis*, double> all_hypothesis;
    auto start_search_time = chrono::high_resolution_clock::now();
    SearchHypotheses(min_start_time_ms, max_finish_time_ms, all_hypothesis, nopenmp_threads);
    vector<pair<double, Hypothesis*> > likelihood_hypothesis;
    for (auto& it: all_hypothesis){
        likelihood_hypothesis.push_back(make_pair(it.second, it.first));
    }
    sort(likelihood_hypothesis.begin(), likelihood_hypothesis.end(),
                                greater<pair<double, Hypothesis*> >());
    localized_links.clear();
    double highest_likelihood = likelihood_hypothesis[0].first;
    set<Hypothesis*> candidate_hypothesis;
    set<int> candidate_link_ids;
    for (int i=min((int)likelihood_hypothesis.size(), N_MAX_K_LIKELIHOODS)-1; i>=0; i--){
        double likelihood = likelihood_hypothesis[i].first;
        Hypothesis *hypothesis = likelihood_hypothesis[i].second;
        if (highest_likelihood - likelihood <= 1.0e-3 and candidate_hypothesis.size() == 0){
            candidate_hypothesis.insert(hypothesis);
            candidate_link_ids.insert(hypothesis->begin(), hypothesis->end());
        }
        if constexpr (VERBOSE){
            cout << "Likely candidate "<<data->IdsToLinks(*hypothesis)<<" "<<likelihood << endl;
        }
    }

    // If multiple hypothesis have highest likelihoods, then return common elements in all hypothesis
    vector<int> erase_link_ids;
    for(int link_id: candidate_link_ids){
        for (auto& ch: candidate_hypothesis){
            if (ch->find(link_id) == ch->end()){
                erase_link_ids.push_back(link_id);
                break;
            }
        }
    }
    for(int link_id: erase_link_ids) candidate_link_ids.erase(link_id);
    localized_links.insert(candidate_link_ids.begin(), candidate_link_ids.end());
    
    if constexpr (VERBOSE){
        cout << endl << "Searched hypothesis space in "
             << GetTimeSinceSeconds(start_search_time) << " seconds" << endl;
        Hypothesis correct_hypothesis;
        data->GetFailedLinkIds(correct_hypothesis);
        Hypothesis* no_failure_hypothesis = new Hypothesis();
        double likelihood_correct_hypothesis = ComputeLogLikelihood(&correct_hypothesis,
					     no_failure_hypothesis, 0.0, min_start_time_ms,
					     max_finish_time_ms, nopenmp_threads);
        if (PRINT_SCORES){
            for (int link_id: localized_links){
                Hypothesis single_link_hypothesis = {link_id}; 
                double l = ComputeLogLikelihood(&single_link_hypothesis, no_failure_hypothesis, 0.0,
                                      min_start_time_ms, max_finish_time_ms, nopenmp_threads);
                //cout << "Score of predicted failed link " << data->inverse_links[link_id] << " score " << drops_per_link[link_id] << " E[nflows] " << flows_per_link[link_id] << " likelihood " << l << endl;
            }
        }
        cout << "Correct Hypothesis " << data->IdsToLinks(correct_hypothesis)
             << " likelihood " << likelihood_correct_hypothesis << endl;
        //VerifyLikelihoodComputation(all_hypothesis, min_start_time_ms,
        //                            max_finish_time_ms, nopenmp_threads);
        delete(no_failure_hypothesis);
    }
    CleanUpAfterLocalization(all_hypothesis);
    if constexpr (VERBOSE) {
        cout << "Finished further search in " << GetTimeSinceSeconds(timer_checkpoint) << " seconds" << endl;
    }
}
