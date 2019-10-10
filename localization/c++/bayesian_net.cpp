#include "bayesian_net.h"
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

// Optimization legitimate only if USE_CONDITIONAL = false
void BayesianNet::ComputeAndStoreIntermediateValues(int nopenmp_threads, double max_finish_time_ms){
    assert(!USE_CONDITIONAL);
    #pragma omp parallel for num_threads(nopenmp_threads)
    for (int ii=0; ii<data->flows.size(); ii++){
        Flow* flow = data->flows[ii];
        flow->SetCachedIntermediateValue(GetBnfWeightedUnconditionalIntermediateValue(
                                         flow, max_finish_time_ms));
    }
}

BayesianNet* BayesianNet::CreateObject(){
    BayesianNet* ret = new BayesianNet();
    vector<double> param {p1, p2};
    ret->SetParams(param);
    return ret;
}

void BayesianNet::SetParams(vector<double>& param) {
    assert(param.size() == 2);
    tie(p1, p2) = {param[0], param[1]};
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
        if (nfails == 1) {
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
            //SortCandidatesWrtNumRelevantFlows(candidates);
        }
        if constexpr (VERBOSE){
            cout << "Finished hypothesis search across "<<result.size()<<" Hypothesis for " << nfails
                 <<" failures in "<< GetTimeSinceMilliSeconds(start_stage_time) << " seconds" << endl;
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
    assert(data != NULL);
    BinFlowsByLinkId(max_finish_time_ms, nopenmp_threads);
    if (not USE_CONDITIONAL){
        auto start_time = chrono::high_resolution_clock::now();
        ComputeAndStoreIntermediateValues(nopenmp_threads, max_finish_time_ms);
        if constexpr (VERBOSE){
            cout << "Finished computing intermediate values in "
                 << GetTimeSinceMilliSeconds(start_time) << " seconds" << endl;
        }
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
    for (int i=min((int)likelihood_hypothesis.size(), N_MAX_K_LIKELIHOODS)-1; i>=0; i--){
        double likelihood = likelihood_hypothesis[i].first;
        Hypothesis *hypothesis = likelihood_hypothesis[i].second;
        if (highest_likelihood - likelihood <= 1.0e-3){
            localized_links.insert(hypothesis->begin(), hypothesis->end());
        }
        if constexpr (VERBOSE){
            cout << "Likely candidate "<<data->IdsToLinks(*hypothesis)<<" "<<likelihood << endl;
        }
    }
    if constexpr (VERBOSE){
        cout << endl << "Searched hypothesis space in "
             << GetTimeSinceMilliSeconds(start_search_time) << " seconds" << endl;
        Hypothesis correct_hypothesis;
        data->GetFailedLinkIds(correct_hypothesis);
        Hypothesis* no_failure_hypothesis = new Hypothesis();
        double likelihood_correct_hypothesis = ComputeLogLikelihood(&correct_hypothesis,
                             no_failure_hypothesis, 0.0, min_start_time_ms, max_finish_time_ms);
        cout << "Correct Hypothesis " << data->IdsToLinks(correct_hypothesis)
             << " likelihood " << likelihood_correct_hypothesis << endl;
        //VerifyLikelihoodComputation(all_hypothesis, min_start_time_ms,
        //                            max_finish_time_ms, nopenmp_threads);
        delete(no_failure_hypothesis);
    }
    CleanUpAfterLocalization(all_hypothesis);
}

bool BayesianNet::VerifyLikelihoodComputation(unordered_map<Hypothesis*, double>& all_hypothesis,
                        double min_start_time_ms, double max_finish_time_ms, int nopenmp_threads){
    int ctr = 0;
    Hypothesis* no_failure_hypothesis = new Hypothesis();
    bool result = true;
    for (auto &[hypothesis_ptr, likelihood] : all_hypothesis){
        if (ctr++ % 10000 == 0) cout << "Finished verifying " << (ctr-1) << " hypotheses" << endl;
        double vanilla = ComputeLogLikelihood(hypothesis_ptr, no_failure_hypothesis, 0.0,
                                  min_start_time_ms, max_finish_time_ms, nopenmp_threads);
        if (abs((vanilla-likelihood)/vanilla) > 1.0e-3 or abs(vanilla-likelihood) > 0.001){
            cout << "Violating hypothesis "<< data->IdsToLinks(*hypothesis_ptr) << " "
                 << likelihood << " != " << vanilla << endl;
            result = false;
        }
    }
    cout << "Finished verifying " << ctr << " hypotheses" << endl;
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
        vector<Path*>* flow_paths = flow->GetPaths(max_finish_time_ms);
        int npaths = flow_paths->size();
        if (REDUCED_ANALYSIS) npaths = flow->npaths_unreduced;
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
        double intermediate_val = flow->GetCachedIntermediateValue();
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
             << GetTimeSinceMilliSeconds(start_init_time) << " seconds" << endl;
    }

    int nlinks = data->inverse_links.size();
    int nfails=1;
    while(nfails <= MAX_FAILS){
        auto start_stage_time = chrono::high_resolution_clock::now();
        assert (top_candidate_hypothesis.size() <= NUM_TOP_HYPOTHESIS_AT_EACH_STAGE);
        //nl_nh_bh_s (new_likelihood, new_hypothesis, base_hypothesis, scores)
        vector<tuple<double, Hypothesis*, Hypothesis*, vector<double>* > > nl_nh_bh_s;
        for (int ii=0; ii<top_candidate_hypothesis.size(); ii++){
            Hypothesis *base_hypothesis = top_candidate_hypothesis[ii];
            double base_likelihood = all_hypothesis[base_hypothesis];
            vector<int> top_link_ids;
            GetIndicesOfTopK(likelihood_scores[ii], NUM_TOP_HYPOTHESIS_AT_EACH_STAGE, top_link_ids, base_hypothesis);
            for (int link_id: top_link_ids){
                assert(base_hypothesis->find(link_id) == base_hypothesis->end());
                //!TODO: check for duplicate hypotheses
                Hypothesis *new_hypothesis = new Hypothesis(*base_hypothesis);
                new_hypothesis->insert(link_id);
                double new_likelihood = base_likelihood + likelihood_scores[ii][link_id]
                                                        + ComputeLogPrior(new_hypothesis)
                                                        - ComputeLogPrior(base_hypothesis);
                nl_nh_bh_s.push_back({new_likelihood, new_hypothesis, base_hypothesis, &likelihood_scores[ii]});
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
                 << " failures in "<< GetTimeSinceMilliSeconds(start_stage_time) << " seconds" << endl;
        }
        nfails++;
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
        if (USE_CONDITIONAL and !flow->TracerouteFlow(max_finish_time_ms)) continue;
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
        double intermediate_val = flow->GetCachedIntermediateValue();
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
        //if (REDUCED_ANALYSIS) naffected = 1;
        double log_likelihood = BnfWeighted(naffected, npaths, naffected_r,
                                           npaths_r, weight.first,
                                           weight.second, intermediate_val);
        likelihoods[thread_num][flow->first_link_id] += log_likelihood;
        likelihoods[thread_num][flow->last_link_id] += log_likelihood;
    }
    initial_likelihoods.resize(nlinks);
    fill(initial_likelihoods.begin(), initial_likelihoods.end(), 0.0);
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
        cout << "Total flows analyzed "<< total_flows_analyzed << " ";
    }
}

inline double BayesianNet::BnfWeighted(int naffected, int npaths, int naffected_r, int npaths_r,
                                       double weight_good, double weight_bad){
    if (USE_CONDITIONAL)
        return BnfWeightedConditional(naffected, npaths, naffected_r, npaths_r, weight_good, weight_bad);
    else
        return BnfWeightedUnconditional(naffected, npaths, naffected_r, npaths_r, weight_good, weight_bad);
}

inline double BayesianNet::BnfWeighted(int naffected, int npaths, int naffected_r, int npaths_r,
                                 double weight_good, double weight_bad, double intermediate_val){
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
    // e2e: end-to-end
    int e2e_paths = npaths * npaths_r;
    int e2e_failed_paths = e2e_paths - (npaths - naffected) * (npaths_r - naffected_r);
    double a = ((double)(e2e_paths - e2e_failed_paths))/e2e_paths;
    double b = ((double)e2e_failed_paths)/e2e_paths;
    double val1 = a + b * pow(((1.0 - p1)/p2), weight_bad) * pow((p1/(1.0-p2)), weight_good);
    double total_weight = weight_good + weight_bad;
    double val2 = a + b * (1 - pow(p1, total_weight))/(1.0 - pow((1.0 - p2), total_weight));
    return log(val1/val2);
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
    return log(a + b * pow((1.0 - p1)/p2, weight_bad) * pow(p1/(1.0-p2), weight_good));
}

inline double BayesianNet::GetBnfWeightedUnconditionalIntermediateValue(Flow *flow, double max_finish_time_ms){
    PII weight = flow->LabelWeightsFunc(max_finish_time_ms);
    assert (!USE_CONDITIONAL);
    // return log((1.0 - a) + a * pow((1.0 - p1)/p2, weight_bad)*pow(p1/(1.0-p2), weight_good));
    //                            <------------------- intermediate_val ---------------------->
    return pow((1.0 - p1)/p2, weight.second) * pow(p1/(1.0-p2), weight.first);
}

inline double BayesianNet::BnfWeightedUnconditionalIntermediate(int naffected, int npaths,
                                   int naffected_r, int npaths_r, double intermediate_val){
    // e2e: end-to-end
    int e2e_paths = npaths * npaths_r;
    int e2e_failed_paths = e2e_paths - (npaths - naffected) * (npaths_r - naffected_r);
    double a = ((double)(e2e_paths - e2e_failed_paths))/e2e_paths;
    double b = ((double)e2e_failed_paths)/e2e_paths;
    return log (a + b * intermediate_val); 
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
                                         double max_finish_time_ms, int nopenmp_threads){
    vector<int> relevant_flows;
    GetRelevantFlows(hypothesis, base_hypothesis, min_start_time_ms, max_finish_time_ms, relevant_flows);
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
