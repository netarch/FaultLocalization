#include "bayesian_net.h"
#include "utils.h"
#include <limits>
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

//!TODO: Change to smart pointers for easier GC
//!TODO: Add code for REVERSE_PATH


// Optimization to ensure that extra links in the hypothesis are the optimal ones
void BayesianNet::SortCandidatesWrtNumRelevantFlows(vector<int> &candidates){
    sort(candidates.begin(), candidates.end(), 
         [&](int & link_id1, int link_id2) -> bool { 
            return flows_by_link_id[link_id1].size() < flows_by_link_id[link_id2].size(); 
         });
}

// Free memory allocated to hypotheses
void BayesianNet::CleanUpAfterLocalization(unordered_map<Hypothesis*, double> &all_hypothesis){
    for (auto& it: all_hypothesis){
        delete(it.first);
    }
}

// Optimization legitimate for regular unconditional computation
void BayesianNet::ComputeAndStoreIntermediateValues(int nopenmp_threads, double max_finish_time_ms){
    #pragma omp parallel for num_threads(nopenmp_threads)
    for (int ii=0; ii<data->flows.size(); ii++){
        Flow* flow = data->flows[ii];
        if (flow->AnySnapshotBefore(max_finish_time_ms)){
            flow->SetCachedIntermediateValue(GetBnfWeightedUnconditionalIntermediateValue(
                                             flow, max_finish_time_ms));
        }
    }
}

BayesianNet* BayesianNet::CreateObject(){
    BayesianNet* ret = new BayesianNet();
    vector<double> param {p1, p2, PRIOR};
    ret->SetParams(param);
    return ret;
}

void BayesianNet::SetParams(vector<double>& param) {
    assert(param.size() == 3);
    tie(p1, p2, PRIOR) = {param[0], param[1], param[2]};
}
    
void BayesianNet::SetLogData(LogData* data_, double max_finish_time_ms, int nopenmp_threads){
    Estimator::SetLogData(data_, max_finish_time_ms, nopenmp_threads);
    if(USE_CONDITIONAL){
        data->FilterFlowsForConditional(max_finish_time_ms, nopenmp_threads);
    }
}

void BayesianNet::SearchHypotheses(double min_start_time_ms, double max_finish_time_ms,
                                    unordered_map<Hypothesis*, double> &all_hypothesis,
                                    int nopenmp_threads){
    Hypothesis* no_failure_hypothesis = new Hypothesis();
    all_hypothesis[no_failure_hypothesis] = 0.0;
    vector<Hypothesis*> prev_hypothesis_space; 
    //Initial list of candidates is all links
    vector<int> candidates;
    for (int link_id=0; link_id<data->inverse_links.size(); link_id++){
        candidates.push_back(link_id);
    }
    prev_hypothesis_space.push_back(no_failure_hypothesis);
    int nfails = 1;
    while (nfails <= MAX_FAILS){
        auto start_stage_time = chrono::high_resolution_clock::now();
        vector<pair<double, Hypothesis*> > result;
        if (nfails == 1 and false) {
            ComputeSingleLinkLogLikelihood(result, min_start_time_ms,
                                       max_finish_time_ms, nopenmp_threads);
        }
        else{
            // Inefficient if |hypothesis_space| is large, say > 1000
            vector<Hypothesis*> hypothesis_space;
            vector<pair<Hypothesis*, double> > base_hypothesis_likelihood;
            for (int link_id: candidates){
                for(Hypothesis* h: prev_hypothesis_space){
                    assert (h != NULL);
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
                if constexpr (VERBOSE){
                    cout << "Candidate " << data->IdsToLinks(*h) << " " << result[i].first << endl;
                }
            }
            if constexpr (VERBOSE) {
                for (auto [link, failparam]: data->failed_links){
                    int link_id = data->links_to_ids[link];
                    if (find(candidates.begin(), candidates.end(), link_id) == candidates.end()){
                        cout << "Link " << link << " absent from initial candidates" << endl;
                    }
                }
            }
            //SortCandidatesWrtNumRelevantFlows(candidates);
        }
        if constexpr (VERBOSE){
            cout << "Finished hypothesis search across "<<result.size()<<" Hypothesis for " << nfails
                 <<" failures in "<< GetTimeSinceSeconds(start_stage_time) << " seconds" << endl;
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
}

void BayesianNet::LocalizeFailures(double min_start_time_ms, double max_finish_time_ms,
                                   Hypothesis& localized_links, int nopenmp_threads){
    if constexpr (VERBOSE) cout << "Params: p1 " << 1.0 - p1 << " p2 " << p2 << " Prior " << PRIOR << " openmp threads " << nopenmp_threads << endl;
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
    SearchHypotheses1(min_start_time_ms, max_finish_time_ms, all_hypothesis, nopenmp_threads);
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
        if constexpr (VERBOSE){
            cout << "Likely candidate "<<data->IdsToLinks(*hypothesis)<<" "<<likelihood << endl;
        }
        if (highest_likelihood - likelihood <= 1.0e-3 and candidate_hypothesis.size() == 0){
            candidate_hypothesis.insert(hypothesis);
            candidate_link_ids.insert(hypothesis->begin(), hypothesis->end());
	        break;
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

bool BayesianNet::VerifyLikelihoodComputation(unordered_map<Hypothesis*, double>& all_hypothesis,
                        double min_start_time_ms, double max_finish_time_ms, int nopenmp_threads){
    int ctr = 0;
    Hypothesis* no_failure_hypothesis = new Hypothesis();
    bool result = true;
    int ctr_network=0;
    for (auto &[hypothesis_ptr, likelihood] : all_hypothesis){
        if (ctr++ % 10000 == 0) cout << "Finished verifying " << (ctr-1) << " hypotheses" << endl;
        for (int link_id: *hypothesis_ptr){
            if(data->inverse_links[link_id].first < 10000 and data->inverse_links[link_id].second < 10000){
                ctr_network++;
                break;
            }
        }
        double vanilla = ComputeLogLikelihood(hypothesis_ptr, no_failure_hypothesis, 0.0,
                                  min_start_time_ms, max_finish_time_ms, nopenmp_threads);
        if (abs((vanilla-likelihood)/vanilla) > 1.0e-3 or abs(vanilla-likelihood) > 0.001){
            cout << "Violating hypothesis "<< data->IdsToLinks(*hypothesis_ptr) << " "
                 << likelihood << " != " << vanilla << endl;
            result = false;
        }
    }
    cout << "Finished verifying " << ctr << " hypotheses" << endl;
    cout << "Finished verifying " << ctr_network << " network hypotheses" << endl;
    delete(no_failure_hypothesis);
    return result;
}

void BayesianNet::GetIndicesOfTopK(vector<double>& scores, int k, vector<int>& result, Hypothesis* exclude){
    int nlinks = data->inverse_links.size();
    assert(scores.size()==nlinks);
    typedef pair<double, int> PDI;
    priority_queue<PDI, vector<PDI>, greater<PDI> > min_q;
    for (int i=0; i<scores.size(); ++i) {
        if (exclude->find(i) == exclude->end()){
            min_q.push(PDI(scores[i], i));
        }
        if(min_q.size() > k){
            min_q.pop();
        }
    }
    assert(result.size()==0);
    result.reserve(min_q.size());
    while(!min_q.empty()){
        auto [score, ind] = min_q.top();
        result.push_back(ind);
        min_q.pop();
    }
}

// Return value for logging purposes
int BayesianNet::UpdateScores(vector<double> &likelihood_scores, Hypothesis* hypothesis,
                               Hypothesis* base_hypothesis, double min_start_time_ms,
                               double max_finish_time_ms, int nopenmp_threads){
    int nlinks = data->inverse_links.size();
    vector<double> likelihood_scores_thread[nopenmp_threads];
    vector<short int> link_ctrs_threads[nopenmp_threads];
    vector<short int> link_ctrs_threads_base[nopenmp_threads];
    #pragma omp parallel for num_threads(nopenmp_threads)
    for(int t=0; t<nopenmp_threads; t++){
        likelihood_scores_thread[t].resize(nlinks, 0.0);
        link_ctrs_threads[t].resize(nlinks, 0);
        link_ctrs_threads_base[t].resize(nlinks, 0);
    }
    auto start_init_time = chrono::high_resolution_clock::now();
    vector<int> relevant_flows;
    GetRelevantFlows(hypothesis, base_hypothesis, min_start_time_ms,
                                max_finish_time_ms, relevant_flows);
    #pragma omp parallel for num_threads(nopenmp_threads) if(nopenmp_threads > 1)
    for (int ii=0; ii<relevant_flows.size(); ii++){
        int thread_num = omp_get_thread_num();
        Flow *flow = data->flows[relevant_flows[ii]];
        if (DiscardFlow(flow, min_start_time_ms, max_finish_time_ms)) continue;
        vector<Path*>* flow_paths = flow->GetPaths(max_finish_time_ms);
        int npaths = flow_paths->size();
        if (REDUCED_ANALYSIS){
            assert (npaths == 1);
            npaths = flow->npaths_unreduced;
        }
        //!TODO: implement REVERSE_PATH case
        int npaths_r = 1, naffected_r = 0, naffected_r_base = 0;
        if constexpr (CONSIDER_REVERSE_PATH) {assert (false);}
        PII weight = flow->LabelWeightsFunc(max_finish_time_ms);
        if (weight.first == 0 and weight.second == 0) continue;
        bool all_paths_failed = (hypothesis->count(flow->first_link_id) > 0) or 
                                (hypothesis->count(flow->last_link_id) > 0);
        bool all_paths_failed_base = (base_hypothesis->count(flow->first_link_id) > 0) or 
                                     (base_hypothesis->count(flow->last_link_id) > 0);
        int npaths_failed = 0, npaths_failed_base = 0;
        for (Path* path: *flow_paths){
            bool fail_path = (HypothesisIntersectsPath(hypothesis, path) or all_paths_failed);
            bool fail_path_base = (HypothesisIntersectsPath(base_hypothesis, path) or all_paths_failed_base);
            npaths_failed += (int) fail_path;
            npaths_failed_base += (int) fail_path_base;
            for (int link_id: *path){
                link_ctrs_threads[thread_num][link_id] += (int) (!fail_path);
                link_ctrs_threads_base[thread_num][link_id] += (int) (!fail_path_base);
            }
        }
        //!TODO: reduced analysis can be made cleaner and more robust
        if (REDUCED_ANALYSIS and all_paths_failed) npaths_failed = npaths; 
        long double intermediate_val = flow->GetCachedIntermediateValue();
        double no_failure_score = BnfWeighted(npaths_failed, npaths, naffected_r, npaths_r,
                                            weight.first, weight.second, intermediate_val);
        double no_failure_score_base = BnfWeighted(npaths_failed_base, npaths, naffected_r_base,
                                        npaths_r, weight.first, weight.second, intermediate_val);
        for (Path* path: *flow_paths){
            for (int link_id: *path){
                int naffected = link_ctrs_threads[thread_num][link_id];
                int naffected_base = link_ctrs_threads_base[thread_num][link_id];
                if (naffected > 0 or naffected_base > 0){
                    double diff = BnfWeighted(naffected + npaths_failed, npaths, naffected_r,
                                     npaths_r, weight.first, weight.second, intermediate_val);
                    diff -= BnfWeighted(naffected_base + npaths_failed_base, npaths,
                                        naffected_r_base, npaths_r, weight.first,
                                        weight.second, intermediate_val);
                    diff -= (no_failure_score - no_failure_score_base);
                    likelihood_scores_thread[thread_num][link_id] += diff;
                    // Reset counter so that likelihood for link_id isn't counted again
                    link_ctrs_threads[thread_num][link_id] = 0;
                    link_ctrs_threads_base[thread_num][link_id] = 0;
                }
            }
        }
        // For the first and last links that are common to all paths
        double diff = -(no_failure_score - no_failure_score_base);
        likelihood_scores_thread[thread_num][flow->first_link_id] += diff;
        likelihood_scores_thread[thread_num][flow->last_link_id] += diff;
    }
    #pragma omp parallel for num_threads(nopenmp_threads)
    for (int link_id=0; link_id<nlinks; link_id++){
        //!TODO: Try changing 2d array from (threads X links) to (links X threads)
        for(int t=0; t<nopenmp_threads; t++){
            likelihood_scores[link_id] += likelihood_scores_thread[t][link_id];
        }
    }
    return relevant_flows.size();
}


void BayesianNet::SearchHypotheses1(double min_start_time_ms, double max_finish_time_ms,
                                    unordered_map<Hypothesis*, double> &all_hypothesis,
                                    int nopenmp_threads){
    Hypothesis* no_failure_hypothesis = new Hypothesis();
    all_hypothesis[no_failure_hypothesis] = 0.0;

    vector<double> likelihood_scores1[NUM_TOP_HYPOTHESIS_AT_EACH_STAGE];
    vector<double> likelihood_scores2[NUM_TOP_HYPOTHESIS_AT_EACH_STAGE];
    vector<double> *likelihood_scores = likelihood_scores1;
    vector<double> *other_likelihood_scores = likelihood_scores2;

    vector<Hypothesis*> top_candidate_hypothesis;
    top_candidate_hypothesis.reserve(NUM_TOP_HYPOTHESIS_AT_EACH_STAGE);
    top_candidate_hypothesis.push_back(no_failure_hypothesis);

    auto start_init_time = chrono::high_resolution_clock::now();
    ComputeInitialLikelihoods(likelihood_scores[0], min_start_time_ms,
                              max_finish_time_ms, nopenmp_threads);
    if constexpr (VERBOSE){
        cout << "Finished hypothesis search for 1 failure in "
             << GetTimeSinceSeconds(start_init_time) << " seconds" << endl;
        cout << "Finished linear stage in " << GetTimeSinceSeconds(timer_checkpoint) << " seconds" << endl;
        timer_checkpoint = chrono::high_resolution_clock::now();
    }
    int nlinks = data->inverse_links.size();
    int nfails=2; // single link failures already handled
    double max_likelihood_this_stage = 0; //empty hypothesis
    double max_likelihood_previous_stage = -1.0e10;
    while(max_likelihood_this_stage > max_likelihood_previous_stage){ // nfails <= MAX_FAILS){
    //while(nfails <= MAX_FAILS){
        max_likelihood_previous_stage = max_likelihood_this_stage;
        max_likelihood_this_stage = -1.0e10; //-inf
        auto start_stage_time = chrono::high_resolution_clock::now();
        assert (top_candidate_hypothesis.size() <= NUM_TOP_HYPOTHESIS_AT_EACH_STAGE);
        //nl_nh_bh_s (new_likelihood, new_hypothesis, base_hypothesis, scores)
        vector<tuple<double, Hypothesis*, Hypothesis*, vector<double>* > > nl_nh_bh_s;
        vector<Hypothesis*> unique_hypotheses;
        for (int ii=0; ii<top_candidate_hypothesis.size(); ii++){
            Hypothesis *base_hypothesis = top_candidate_hypothesis[ii];
            double base_likelihood = all_hypothesis[base_hypothesis];
            vector<int> top_link_ids;
            GetIndicesOfTopK(likelihood_scores[ii], NUM_TOP_HYPOTHESIS_AT_EACH_STAGE, top_link_ids, base_hypothesis);
            for (int link_id: top_link_ids){
                assert(base_hypothesis->find(link_id) == base_hypothesis->end());
                base_hypothesis->insert(link_id);
                if (find_if(unique_hypotheses.begin(), unique_hypotheses.end(),
                        HypothesisPointerCompare(base_hypothesis)) == unique_hypotheses.end()){
                    Hypothesis *new_hypothesis = new Hypothesis(*base_hypothesis);
                    base_hypothesis->erase(link_id);
                    unique_hypotheses.push_back(new_hypothesis);
                    double new_likelihood = base_likelihood + likelihood_scores[ii][link_id]
                                                            + ComputeLogPrior(new_hypothesis)
                                                            - ComputeLogPrior(base_hypothesis);
                    max_likelihood_this_stage = max(new_likelihood, max_likelihood_this_stage);
                    nl_nh_bh_s.push_back({new_likelihood, new_hypothesis, base_hypothesis, &likelihood_scores[ii]});
                }
                else base_hypothesis->erase(link_id);
            }
        }
        sort(nl_nh_bh_s.begin(), nl_nh_bh_s.end(), greater<tuple<double, Hypothesis*, Hypothesis*, vector<double>*> >());
        top_candidate_hypothesis.clear();
        int num_flows_analyzed = 0;
        for (int ii=0; ii<min((int)nl_nh_bh_s.size(), NUM_TOP_HYPOTHESIS_AT_EACH_STAGE); ii++){
            auto &[likelihood, hypothesis, base_hypothesis, scores] = nl_nh_bh_s[ii];
            all_hypothesis[hypothesis] = likelihood;
            top_candidate_hypothesis.push_back(hypothesis);
            //Update scores for other_likelihood_scores
            //cout << "After update scores " << data->IdsToLinks(*hypothesis) << " " << data->IdsToLinks(*base_hypothesis) << endl;
            assert(scores->size() == nlinks);
            other_likelihood_scores[ii].clear();
            other_likelihood_scores[ii].insert(other_likelihood_scores[ii].begin(), scores->begin(), scores->end());
            num_flows_analyzed += UpdateScores(other_likelihood_scores[ii], hypothesis, base_hypothesis,
                                                min_start_time_ms, max_finish_time_ms, nopenmp_threads);
        }
        swap(likelihood_scores, other_likelihood_scores);
        if constexpr (VERBOSE){
            cout << "Total flows analyzed " << num_flows_analyzed
                 << " Finished hypothesis search for " << nfails
                 << " failures in "<< GetTimeSinceSeconds(start_stage_time) << " seconds" << endl;
        }
        nfails++;
    }
}

void BayesianNet::PrintScores(double min_start_time_ms, double max_finish_time_ms, int nopenmp_threads){
    int nlinks = data->inverse_links.size();
    vector<double> scores1[nopenmp_threads];
    vector<double> scores2[nopenmp_threads];
    vector<double> nflows_threads[nopenmp_threads];
    vector<short int> link_ctrs_threads[nopenmp_threads];
    #pragma omp parallel for num_threads(nopenmp_threads)
    for(int t=0; t<nopenmp_threads; t++){
        scores1[t].resize(nlinks, 0.0);
        scores2[t].resize(nlinks, 0.0);
        nflows_threads[t].resize(nlinks, 0.0);
        link_ctrs_threads[t].resize(nlinks, 0);
    }
    #pragma omp parallel for num_threads(nopenmp_threads)
    for (int ii=0; ii<data->flows.size(); ii++){
        Flow* flow = data->flows[ii];
        if (DiscardFlow(flow, min_start_time_ms, max_finish_time_ms)) continue;
        int thread_num = omp_get_thread_num();
        vector<Path*>* flow_paths = flow->GetPaths(max_finish_time_ms);
        int npaths = flow_paths->size();
        if (REDUCED_ANALYSIS) npaths = flow->npaths_unreduced;
        //!TODO: implement REVERSE_PATH case
        int npaths_r = 1, naffected_r = 0;
        if constexpr (CONSIDER_REVERSE_PATH) {assert (false);}
        PII weight = flow->LabelWeightsFunc(max_finish_time_ms);
        if (weight.first == 0 and weight.second == 0) continue;
        nflows_threads[thread_num][flow->first_link_id] += 1;
        nflows_threads[thread_num][flow->last_link_id] += 1;
        scores1[thread_num][flow->first_link_id] += weight.first;
        scores2[thread_num][flow->first_link_id] += weight.second;
        scores1[thread_num][flow->last_link_id] += weight.first;
        scores2[thread_num][flow->last_link_id] += weight.second;
        /*
        if (flow->dest == 10278){
            flow->PrintFlowMetrics();
            cout << "weights " << weight.first << " " << weight.second << endl;
        }
        */
        //cout << "npaths " << npaths << endl;
        for (Path* path: *flow_paths){
            for (int link_id: *path){
                link_ctrs_threads[thread_num][link_id]++;
            }
        }
        for (Path* path: *flow_paths){
            for (int link_id: *path){
                int naffected = link_ctrs_threads[thread_num][link_id];
                if (naffected > 0){
                    nflows_threads[thread_num][link_id] += ((double)naffected)/npaths;
                    scores1[thread_num][link_id] += (double)(weight.first * naffected)/npaths;
                    scores2[thread_num][link_id] += weight.second;
                }
                // Reset counter so that likelihood for link_id isn't counted again
                link_ctrs_threads[thread_num][link_id] = 0;
            }
        }
    }
    drops_per_link.resize(nlinks, 0.0);
    flows_per_link.resize(nlinks, 0.0);

    auto GetScore =
        [&scores1, &scores2, nopenmp_threads] (int link_id){
            double score1=0, score2=0;
            for(int t=0; t<nopenmp_threads; t++){
                score1 += scores1[t][link_id];
                score2 += scores2[t][link_id];
            }
            return tuple(score2, score1);
        };
    #pragma omp parallel for num_threads(nopenmp_threads)
    for (int link_id=0; link_id<nlinks; link_id++){
        //!TODO: Try changing likelihoods 2d array from (threads X links) to (links X threads)
        auto [score2, score1] = GetScore(link_id);
        drops_per_link[link_id] = score2/score1;
        double f = 0.0;
        for(int t=0; t<nopenmp_threads; t++){
            f += nflows_threads[t][link_id];
        }
        flows_per_link[link_id] = f;
    }
    vector<int> idx(drops_per_link.size());
    iota(idx.begin(), idx.end(), 0);
    sort(idx.begin(), idx.end(), [this](int i1, int i2) {return (abs(drops_per_link[i1] - drops_per_link[i2]) < 1.0e-6? false: (drops_per_link[i1] < drops_per_link[i2]));});
    for (int ii = max(0, (int)idx.size()-50); ii<idx.size(); ii++){
        int link_id = idx[ii];
        auto [score2, score1] = GetScore(link_id);
        cout << "Link " << data->inverse_links[link_id] << " " << score2 << " " << score1 << " " << drops_per_link[link_id] << " E[nflows] " << flows_per_link[link_id] << endl;
    }
    for (auto [link, failparam]: data->failed_links){
        int link_id = data->links_to_ids[link];
        auto [score2, score1] = GetScore(link_id);
        cout << "Failed Link " << link << " " << score2 << " " << score1 << " " << drops_per_link[link_id] << " E[nflows] " << flows_per_link[link_id] << endl;
    }
}


void BayesianNet::ComputeInitialLikelihoods(vector<double> &initial_likelihoods,
                                        double min_start_time_ms, double max_finish_time_ms,
                                        int nopenmp_threads){
    int nlinks = data->inverse_links.size();
    vector<double> likelihoods[nopenmp_threads];
    vector<short int> link_ctrs_threads[nopenmp_threads];
    #pragma omp parallel for num_threads(nopenmp_threads)
    for(int t=0; t<nopenmp_threads; t++){
        likelihoods[t].resize(nlinks, 0.0);
        link_ctrs_threads[t].resize(nlinks, 0);
    }
    #pragma omp parallel for num_threads(nopenmp_threads)
    for (int ii=0; ii<data->flows.size(); ii++){
        Flow* flow = data->flows[ii];
        if (DiscardFlow(flow, min_start_time_ms, max_finish_time_ms)) continue;
        int thread_num = omp_get_thread_num();
        vector<Path*>* flow_paths = flow->GetPaths(max_finish_time_ms);
        int npaths = flow_paths->size();
        if (REDUCED_ANALYSIS) npaths = flow->npaths_unreduced;
        //!TODO: implement REVERSE_PATH case
        int npaths_r = 1, naffected_r = 0;
        if constexpr (CONSIDER_REVERSE_PATH) {assert (false);}
        PII weight = flow->LabelWeightsFunc(max_finish_time_ms);
        if (weight.first == 0 and weight.second == 0) continue;
        for (Path* path: *flow_paths){
            for (int link_id: *path){
                link_ctrs_threads[thread_num][link_id]++;
            }
        }
        long double intermediate_val = flow->GetCachedIntermediateValue();
        for (Path* path: *flow_paths){
            for (int link_id: *path){
                int naffected = link_ctrs_threads[thread_num][link_id];
                if (naffected > 0){
                    likelihoods[thread_num][link_id] += BnfWeighted(naffected, npaths, naffected_r,
                                                                  npaths_r, weight.first, 
                                                                  weight.second, intermediate_val);
                }
                // Reset counter so that likelihood for link_id isn't counted again
                link_ctrs_threads[thread_num][link_id] = 0;
            }
        }

        // For the first and last links that are common to all paths
        int naffected = npaths;
        //if (REDUCED_ANALYSIS) naffected = npaths;
        double log_likelihood = BnfWeighted(naffected, npaths, naffected_r,
                                           npaths_r, weight.first,
                                           weight.second, intermediate_val);
        if (log_likelihood > 1.0e6){
            cout << "Likelihood computation precision bug ";
            cout << naffected << " " << npaths << " " << naffected_r << " " << npaths_r << " " << weight.first << " " << weight.second << " " << intermediate_val << " " << p1 << " " << p2 << " " << PRIOR << endl;
            //!TODO; TODO: TODO
            assert(false);
        }
        likelihoods[thread_num][flow->first_link_id] += log_likelihood;
        likelihoods[thread_num][flow->last_link_id] += log_likelihood;
    }
    initial_likelihoods.resize(nlinks, 0.0);
    #pragma omp parallel for num_threads(nopenmp_threads)
    for (int link_id=0; link_id<nlinks; link_id++){
        //!TODO: Try changing likelihoods 2d array from (threads X links) to (links X threads)
        for(int t=0; t<nopenmp_threads; t++){
            initial_likelihoods[link_id] += likelihoods[t][link_id];
        }
    }
}

void BayesianNet::ComputeSingleLinkLogLikelihood(vector<pair<double, Hypothesis*> > &result,
                                        double min_start_time_ms, double max_finish_time_ms,
                                        int nopenmp_threads){
    int nlinks = data->inverse_links.size();
    vector<double> initial_likelihoods;
    ComputeInitialLikelihoods(initial_likelihoods, min_start_time_ms,
                                 max_finish_time_ms, nopenmp_threads);
    result.resize(nlinks);
    for(int link_id=0; link_id<nlinks; link_id++){
        Hypothesis* h = new Hypothesis();
        h->insert(link_id);
        double likelihood = initial_likelihoods[link_id];
        likelihood += ComputeLogPrior(h);
        result[link_id] = make_pair(likelihood, h);
    }
}

void BayesianNet::ComputeLogLikelihood(vector<Hypothesis*> &hypothesis_space,
                 vector<pair<Hypothesis*, double> > &base_hypothesis_likelihood,
                 vector<pair<double, Hypothesis*> > &result,
                 double min_start_time_ms, double max_finish_time_ms,
                 int nopenmp_threads){
    result.resize(hypothesis_space.size());
    vector<int> relevant_flows[hypothesis_space.size()];
    #pragma omp parallel for num_threads(nopenmp_threads)
    for(int i=0; i<hypothesis_space.size(); i++){
        Hypothesis* h = hypothesis_space[i];
        auto& base = base_hypothesis_likelihood[i];
        GetRelevantFlows(h, base.first, min_start_time_ms, max_finish_time_ms, relevant_flows[i]);
    }
    int total_flows_analyzed = 0;
    for(int i=0; i<hypothesis_space.size(); i++){
        Hypothesis* h = hypothesis_space[i];
        auto& base = base_hypothesis_likelihood[i];
        result[i] = pair<double, Hypothesis*>(ComputeLogLikelihood(h, base.first, base.second,
                min_start_time_ms, max_finish_time_ms, relevant_flows[i], nopenmp_threads), h);
        total_flows_analyzed += relevant_flows[i].size();
    }
    if constexpr (VERBOSE) {
        //cout << "Total flows analyzed "<< total_flows_analyzed << " ";
    }
}

inline double BayesianNet::BnfWeighted(int naffected, int npaths, int naffected_r,
                              int npaths_r, double weight_good, double weight_bad){
    if (USE_CONDITIONAL)
        return BnfWeightedConditional(naffected, npaths, naffected_r,
                                      npaths_r, weight_good, weight_bad);
    else
        return BnfWeightedUnconditional(naffected, npaths, naffected_r,
                                        npaths_r, weight_good, weight_bad);
}

inline double BayesianNet::BnfWeighted(int naffected, int npaths, int naffected_r,
                                       int npaths_r, double weight_good,
                                       double weight_bad, long double intermediate_val){
    if (USE_CONDITIONAL)
        return BnfWeightedConditional(naffected, npaths, naffected_r,
                                      npaths_r, weight_good, weight_bad);
    else
        return BnfWeightedUnconditionalIntermediate(naffected, npaths,
                                naffected_r, npaths_r, intermediate_val);
}

inline double BayesianNet::BnfWeightedConditional(int naffected, int npaths,
                                                  int naffected_r, int npaths_r,
                                                  double weight_good, double weight_bad){
    //!TODO: change temporary!
    return BnfWeightedUnconditional(naffected, npaths, naffected_r, npaths_r, weight_good, weight_bad);
    // e2e: end-to-end
    int e2e_paths = npaths * npaths_r;
    int e2e_failed_paths = e2e_paths - (npaths - naffected) * (npaths_r - naffected_r);
    assert(e2e_paths == 1);
    double a = ((double)(e2e_paths - e2e_failed_paths))/e2e_paths;
    double b = ((double)e2e_failed_paths)/e2e_paths;
    //!TODO TODO TODO: precision goes out of long double range
    long double ival = pow((long double)((1.0 - p1)/p2), weight_bad) * pow(p1/(1.0-p2), weight_good);
    //ival = min(ival,  std::numeric_limits<long double>::max());
    long double val1 = a + b * ival;
    double total_weight = weight_good + weight_bad;
    double val2 = a + b * (1 - pow(p1, total_weight))/(1.0 - pow((1.0 - p2), total_weight));
    if (val1 >= 1.0e10000 or val2 <= 1.0e-10000){
        cout << "Something went wrong " << val1 << " " <<val2 << " " << weight_bad << " " << weight_good << " " << a << " " << b << endl;
        cout << pow(((1.0 - p1)/p2), weight_bad) << " " << pow((p1/(1.0-p2)), weight_good) << endl;
        cout << ((1.0 - p1)/p2) << " " << (p1/(1.0-p2)) << endl;
        cout << pow(10.0, 513) << endl;
        assert(false);
    }
    return log(val1) - log(val2);
}

inline double BayesianNet::BnfWeightedUnconditional(int naffected, int npaths,
                                                    int naffected_r, int npaths_r,
                                                    double weight_good, double weight_bad){
    // e2e: end-to-end
    int e2e_paths = npaths * npaths_r;
    int e2e_failed_paths = e2e_paths - (npaths - naffected) * (npaths_r - naffected_r);
    // Separate computation for "a" and "b" required because if "a = 0"
    // then using "(1.0 - b)" for "a" inside the log can be buggy
    // as it can get approximated by a small value, which messes with the log computation
    double a = ((double)(e2e_paths - e2e_failed_paths))/e2e_paths;
    double b = ((double)e2e_failed_paths)/e2e_paths;
    return log(a + b * pow((long double)((1.0 - p1)/p2), weight_bad) * pow(p1/(1.0-p2), weight_good));
}

inline long double BayesianNet::GetBnfWeightedUnconditionalIntermediateValue(Flow *flow, double max_finish_time_ms){
    PII weight = flow->LabelWeightsFunc(max_finish_time_ms);
    assert (!USE_CONDITIONAL);
    // return log((1.0 - a) + a * pow((1.0 - p1)/p2, weight_bad)*pow(p1/(1.0-p2), weight_good));
    //                            <------------------- intermediate_val ---------------------->
    long double ret = pow((long double)((1.0 - p1)/p2), weight.second) * pow(p1/(1.0-p2), weight.first);
    //!TODO TODO TODO: precision goes out of long double range
    ret = min(ret,  std::numeric_limits<long double>::max());
    return ret;
}

inline double BayesianNet::BnfWeightedUnconditionalIntermediate(int naffected, int npaths,
                                   int naffected_r, int npaths_r, long double intermediate_val){
    // e2e: end-to-end
    int e2e_paths = npaths * npaths_r;
    int e2e_failed_paths = e2e_paths - (npaths - naffected) * (npaths_r - naffected_r);
    double a = ((double)(e2e_paths - e2e_failed_paths))/e2e_paths;
    double b = ((double)e2e_failed_paths)/e2e_paths;
    return log (a + b * intermediate_val); 
}

bool BayesianNet::DiscardFlow(Flow *flow, double min_start_time_ms, double max_finish_time_ms){
    return (!flow->AnySnapshotBefore(max_finish_time_ms) or flow->start_time_ms < min_start_time_ms or
           (USE_CONDITIONAL and !flow->TracerouteFlow(max_finish_time_ms)));
}

void BayesianNet::GetRelevantFlows(Hypothesis* hypothesis, Hypothesis* base_hypothesis,
                                   double min_start_time_ms, double max_finish_time_ms,
                                   vector<int>& relevant_flows){
    auto start_init_time = chrono::high_resolution_clock::now();
    dense_hash_set<int, hash<int> > relevant_flows_set;
    relevant_flows_set.set_empty_key(-1);
    for (int link_id: *hypothesis){
        if (base_hypothesis->count(link_id) == 0){
            vector<int> &link_id_flows = (*flows_by_link_id)[link_id];
            for(int f: link_id_flows){
                Flow* flow = data->flows[f];
                //!TODO: Add option for active_flows_only
                if (!DiscardFlow(flow, min_start_time_ms, max_finish_time_ms)){
                    relevant_flows_set.insert(f);
                }
            }
        }
    }
    relevant_flows.insert(relevant_flows.begin(), relevant_flows_set.begin(), relevant_flows_set.end());
}

double BayesianNet::ComputeLogLikelihood(Hypothesis* hypothesis, Hypothesis* base_hypothesis,
                                         double base_likelihood, double min_start_time_ms,
                                         double max_finish_time_ms, int nopenmp_threads){
    vector<int> relevant_flows;
    GetRelevantFlows(hypothesis, base_hypothesis, min_start_time_ms, max_finish_time_ms, relevant_flows);
    //cout << " Relevant flows for ComputeLogLikelihood " << relevant_flows.size() 
    //     << " Num flows "<< data->flows.size() << " relevant flows "<< (*flows_by_link_id)[*hypothesis->begin()].size() << endl;  
    return ComputeLogLikelihood(hypothesis, base_hypothesis, base_likelihood,
             min_start_time_ms, max_finish_time_ms, relevant_flows, nopenmp_threads);
}

array<int, 6> BayesianNet::ComputeFlowPathCountersReduced(Flow *flow, Hypothesis *hypothesis,
                                           Hypothesis *base_hypothesis, double max_finish_time_ms){
    vector<Path*>* flow_paths = flow->GetPaths(max_finish_time_ms);
    assert (flow_paths != NULL and flow_paths->size() == 1);
    int npaths = flow->npaths_unreduced, naffected=0, naffected_base=0;
    // If common links of all paths are in hypothesis, then all paths are affected
    if (hypothesis->count(flow->first_link_id) > 0 or hypothesis->count(flow->last_link_id) > 0){
        naffected = npaths;
    }
    else{
        for (Path* path: *flow_paths){
            naffected += HypothesisIntersectsPath(hypothesis, path);
        }
    }
    if (base_hypothesis->count(flow->first_link_id) > 0 or
        base_hypothesis->count(flow->last_link_id) > 0){
        naffected_base = npaths;
    }
    else{
        for (Path* path: *flow_paths){
            naffected_base += HypothesisIntersectsPath(base_hypothesis, path);
        }
    }
    int npaths_r = 1, naffected_r = 0, naffected_base_r = 0;
    //!TODO: implement REVERSE_PATH case
    if constexpr (CONSIDER_REVERSE_PATH) { assert (false); }
    return {npaths, naffected, naffected_base, npaths_r, naffected_r, naffected_base_r};
}

array<int, 6> BayesianNet::ComputeFlowPathCountersUnreduced(Flow *flow, Hypothesis *hypothesis,
                                        Hypothesis *base_hypothesis, double max_finish_time_ms){
    vector<Path*>* flow_paths = flow->GetPaths(max_finish_time_ms);
    assert (flow_paths != NULL);
    int npaths = flow_paths->size(), naffected=0, naffected_base=0;
    // If common end links are in hypothesis, then all paths are affected
    if (hypothesis->count(flow->first_link_id) > 0 or hypothesis->count(flow->last_link_id) > 0){
        naffected = npaths;
    }
    else{
        for (Path* path: *flow_paths){
            naffected += (int) HypothesisIntersectsPath(hypothesis, path);
        }
    }
    if (base_hypothesis->count(flow->first_link_id) > 0 or
        base_hypothesis->count(flow->last_link_id) > 0){
        naffected_base = npaths;
    }
    else{
        for (Path* path: *flow_paths){
            naffected_base += (int) HypothesisIntersectsPath(base_hypothesis, path);
        }
    }
    int npaths_r = 1, naffected_r = 0, naffected_base_r = 0;
    //!TODO: implement REVERSE_PATH case
    if constexpr (CONSIDER_REVERSE_PATH){ assert (false); }
    return {npaths, naffected, naffected_base, npaths_r, naffected_r, naffected_base_r};
}

double BayesianNet::ComputeLogLikelihood2(Hypothesis* hypothesis, Hypothesis* base_hypothesis,
                    double base_likelihood, double min_start_time_ms, double max_finish_time_ms,
                    vector<int> &relevant_flows, int nopenmp_threads){
    vector<double> log_likelihoods_threads(nopenmp_threads, 0.0);
    #pragma omp parallel for num_threads(nopenmp_threads) if(nopenmp_threads > 1)
    for (int ii=0; ii<relevant_flows.size(); ii++){
        Flow *flow = data->flows[relevant_flows[ii]];
        double log_likelihood = 0.0;
        PII weight = flow->LabelWeightsFunc(max_finish_time_ms);
        if (weight.first == 0 and weight.second == 0) continue;
        //!TODO Reduced Analaysis
        assert (!REDUCED_ANALYSIS);
        //!TODO: implement REVERSE_PATH case
        if constexpr (CONSIDER_REVERSE_PATH){ assert (false); }
        vector<Path*>* flow_paths = flow->GetPaths(max_finish_time_ms);
        assert (flow_paths != NULL);
        int npaths = flow_paths->size();
        long double ll = 0.0, ll_base = 0.0;
        long double flow_dir = pow((long double)1.0 - p1, weight.second) * pow ((long double) p1, weight.first);
        flow_dir /= pow((long double)p2, weight.second) * pow ((long double)1.0 - p2, weight.first);
        for (Path* path: *flow_paths){
            int nfailed = (hypothesis->count(flow->first_link_id) > 0);
            nfailed += (hypothesis->count(flow->last_link_id) > 0);
            int nfailed_base = (base_hypothesis->count(flow->first_link_id) > 0);
            nfailed_base += (base_hypothesis->count(flow->last_link_id) > 0);
            for (int link_id: *path){
                nfailed += (hypothesis->count(link_id) > 0);
                nfailed_base += (base_hypothesis->count(link_id) > 0);
            }
            /*
            if (flow_dir >= 1.0){
                double p_bad = (nfailed > 0 ? 1.0 - p1 : p2);
                double p_bad_base = (nfailed_base > 0 ? 1.0 - p1 : p2);
                ll += (pow((long double)p_bad, weight.second) * pow((long double)(1.0 - p_bad), weight.first))/npaths;
                ll_base += (pow((long double)p_bad_base, weight.second) * pow((long double)(1.0 - p_bad_base), weight.first))/npaths;
            }
            else{
                long double no_failure_score = pow((long double)p2, weight.second) * pow ((long double)1.0 - p2, weight.first);
                long double one_failure_score = flow_dir * no_failure_score;
                ll += no_failure_score * pow((long double) flow_dir, nfailed)/npaths;
                ll_base += no_failure_score * pow((long double) flow_dir, nfailed_base)/npaths;
                //ll += (nfailed * (one_failure_score  - no_failure_score) + no_failure_score)/npaths;
                //ll_base += (nfailed_base * (one_failure_score  - no_failure_score) + no_failure_score)/npaths;
            }
            */
            double p_bad = 1.0 - (pow(p1, nfailed) * pow(1.0 - p2, (path->size() + 2 - nfailed)));
            double p_bad_base = 1.0 - (pow(p1, nfailed_base) * pow(1.0 - p2, (path->size() + 2 - nfailed_base)));
            ll += (pow((long double)p_bad, weight.second) * pow((long double)(1.0 - p_bad), weight.first))/npaths;
            ll_base += (pow((long double)p_bad_base, weight.second) * pow((long double)(1.0 - p_bad_base), weight.first))/npaths;
        }
        log_likelihoods_threads[omp_get_thread_num()] += (log(ll) - log(ll_base));
    }
    double log_likelihood = max(-1.0e9, accumulate(log_likelihoods_threads.begin(),
                                         log_likelihoods_threads.end(), 0.0, plus<double>()));
    //cout << *hypothesis << " " << log_likelihood + hypothesis->size() * PRIOR << endl;
    return base_likelihood + log_likelihood + ComputeLogPrior(hypothesis) - ComputeLogPrior(base_hypothesis);
}

double BayesianNet::ComputeLogLikelihood(Hypothesis* hypothesis, Hypothesis* base_hypothesis,
                    double base_likelihood, double min_start_time_ms, double max_finish_time_ms,
                    vector<int> &relevant_flows, int nopenmp_threads){
    vector<double> log_likelihoods_threads(nopenmp_threads, 0.0);
    #pragma omp parallel for num_threads(nopenmp_threads) if(nopenmp_threads > 1)
    for (int ii=0; ii<relevant_flows.size(); ii++){
        Flow *flow = data->flows[relevant_flows[ii]];
        double log_likelihood = 0.0;
        PII weight = flow->LabelWeightsFunc(max_finish_time_ms);
        if (weight.first == 0 and weight.second == 0) continue;
        array<int, 6> path_ctrs;
        if (REDUCED_ANALYSIS){
            path_ctrs = ComputeFlowPathCountersReduced(flow, hypothesis,
                                   base_hypothesis, max_finish_time_ms);
        }
        else{
            path_ctrs = ComputeFlowPathCountersUnreduced(flow, hypothesis,
                                      base_hypothesis, max_finish_time_ms);
        }
        auto [npaths, naffected, naffected_base, npaths_r, naffected_r, naffected_base_r] = path_ctrs;
        int thread_num = omp_get_thread_num();
        log_likelihoods_threads[thread_num] += BnfWeighted(naffected, npaths, naffected_r,
                                                    npaths_r, weight.first, weight.second);
        if (naffected_base > 0 or naffected_base_r > 0){
            log_likelihoods_threads[thread_num] -= BnfWeighted(naffected_base, npaths, naffected_base_r,
                                                               npaths_r, weight.first, weight.second);
        }
    }
    double log_likelihood = max(-1.0e9, accumulate(log_likelihoods_threads.begin(),
                                         log_likelihoods_threads.end(), 0.0, plus<double>()));
    //cout << *hypothesis << " " << log_likelihood + hypothesis->size() * PRIOR << endl;
    return base_likelihood + log_likelihood + ComputeLogPrior(hypothesis) - ComputeLogPrior(base_hypothesis);
}

double BayesianNet::ComputeLogPrior(Hypothesis* hypothesis){
    if (REDUCED_ANALYSIS){
        double log_prior = 0.0;
        for (int link_id: *hypothesis){
            log_prior += log(num_reduced_links_map->at(link_id)) + PRIOR;
        }
        return log_prior;
    }
    else{
        return hypothesis->size() * PRIOR;
    }
}

