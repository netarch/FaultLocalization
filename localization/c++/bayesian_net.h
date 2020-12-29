#ifndef __FAULT_LOCALIZE_BAYESIAN_NET__
#define __FAULT_LOCALIZE_BAYESIAN_NET__
#include "estimator.h"

class BayesianNet : public Estimator{
 public:
    BayesianNet() : Estimator() {}
    void LocalizeFailures(double min_start_time_ms, double max_finish_time_ms,
                          Hypothesis &localized_links, int nopenmp_threads);
    const bool PRINT_SCORES = true;
    const int MAX_FAILS = 20;
    const int NUM_CANDIDATES = max(15, 5 * MAX_FAILS);
    const int NUM_TOP_HYPOTHESIS_AT_EACH_STAGE = 10;
    // For printing purposes
    const int N_MAX_K_LIKELIHOODS = 20;
    const bool USE_CONDITIONAL = false;
    double PRIOR = -1000.0;
    bool REDUCED_ANALYSIS = false;
    void SetReducedAnalysis(bool val) { REDUCED_ANALYSIS = val; }
    void SetNumReducedLinksMap(unordered_map<int, int>* num_reduced_links_map_) {
        num_reduced_links_map = num_reduced_links_map_;
    }
    void SetFlowsByLinkId(vector<vector<int> >* flows_by_link_id_){
        assert (flows_by_link_id_ != NULL);
        flows_by_link_id = flows_by_link_id_;
    }
    BayesianNet* CreateObject();
    void SetLogData(LogData *data, double max_finish_time_ms, int nopenmp_threads);
    void SetParams(vector<double>& param);

private:
    void SearchHypotheses(double min_start_time_ms, double max_finish_time_ms,
                          unordered_map<Hypothesis*, double> &all_hypothesis,
                          int nopenmp_threads);
    void SearchHypotheses1(double min_start_time_ms, double max_finish_time_ms,
                          unordered_map<Hypothesis*, double> &all_hypothesis,
                          int nopenmp_threads);

    void PrintScores(double min_start_time_ms, double max_finish_time_ms,
                     int nopenmp_threads);

    void ComputeInitialLikelihoods(vector<double> &initial_likelihoods,
                                 double min_start_time_ms, double max_finish_time_ms,
                                 int nopenmp_threads);
    void ComputeSingleLinkLogLikelihood(vector<pair<double, Hypothesis*> > &result,
                                 double min_start_time_ms, double  max_finish_time_ms,
                                 int nopenmp_threads);
    void GetRelevantFlows(Hypothesis* hypothesis, Hypothesis* base_hypothesis,
                                 double min_start_time_ms, double max_finish_time_ms,
                                 vector<int>& relevant_flows);

    inline bool DiscardFlow(Flow *flow, double min_start_time_ms, double max_finish_time_ms);

    array<int, 6> ComputeFlowPathCountersUnreduced(Flow *flow, Hypothesis *hypothesis,
                              Hypothesis *base_hypothesis, double max_finish_time_ms);
    array<int, 6> ComputeFlowPathCountersReduced(Flow *flow, Hypothesis *hypothesis,
                              Hypothesis *base_hypothesis, double max_finish_time_ms);

    // for a single hypothesis
    double ComputeLogLikelihood(Hypothesis* hypothesis, Hypothesis* base_hypothesis,
                   double base_likelihood, double min_start_time_ms, double max_finish_time_ms,
                   vector<int> &relevant_flows, int nopenmp_threads=1);

    double ComputeLogLikelihood2(Hypothesis* hypothesis, Hypothesis* base_hypothesis,
                   double base_likelihood, double min_start_time_ms, double max_finish_time_ms,
                   vector<int> &relevant_flows, int nopenmp_threads=1);


    // computes relevant_flows and calls the other version
    double ComputeLogLikelihood(Hypothesis* hypothesis, Hypothesis* base_hypothesis,
                    double base_likelihood, double min_start_time_ms,
                    double max_finish_time_ms, int nopenmp_threads=1);

    // for a set of hypothesis
    void ComputeLogLikelihood(vector<Hypothesis*> &hypothesis_space,
                    vector<pair<Hypothesis*, double> > &base_hypothesis_likelihood,
                    vector<pair<double, Hypothesis*> > &result,
                    double min_start_time_ms, double  max_finish_time_ms,
                    int nopenmp_threads);

    inline long double GetBnfWeightedUnconditionalIntermediateValue(Flow *flow,
                                                double max_finish_time_ms);

    inline double BnfWeighted(int naffected, int npaths, int naffected_r,
                    int npaths_r, double weight_good, double weight_bad);

    inline double BnfWeighted(int naffected, int npaths, int naffected_r,
                              int npaths_r, double weight_good,
                              double weight_bad, long double intermediate_val);

    inline double BnfWeightedConditional(int naffected, int npaths, int naffected_r,
                    int npaths_r, double weight_good, double weight_bad);

    inline double BnfWeightedUnconditional(int naffected, int npaths, int naffected_r,
                    int npaths_r, double weight_good, double weight_bad);

    inline double BnfWeightedUnconditionalIntermediate(int naffected, int npaths,
                         int naffected_r, int npaths_r, long double intermediate_val);

    double ComputeLogPrior(Hypothesis *hypothesis);
    void SortCandidatesWrtNumRelevantFlows(vector<int> &candidates);
    void CleanUpAfterLocalization(unordered_map<Hypothesis*, double> &all_hypothesis);
    
    // Cache intermediate results of time consuming arithmetic operations
    void ComputeAndStoreIntermediateValues(int nopenmp_threads, double max_finish_time_ms);

    int UpdateScores(vector<double> &likelihood_scores, Hypothesis* hypothesis,
                      Hypothesis* base_hypothesis, double min_start_time_ms,
                      double max_finish_time_ms, int nopenmp_threads);
    void GetIndicesOfTopK(vector<double>& scores, int k, vector<int>& result, Hypothesis *exclude);

    // Verify likelihood computation optimizations with the vanilla version
    bool VerifyLikelihoodComputation(unordered_map<Hypothesis*, double>& all_hypothesis,
              double min_start_time_ms, double max_finish_time_ms, int nopenmp_threads);

    // Noise parameters
    double p1 = 1.0-5.0e-3, p2 = 2.0e-4;
    //double p1 = 1.0-4.0e-3, p2 = 1.5e-4;
    // For reduced analysis
    unordered_map<int, int>* num_reduced_links_map;

    // For printing scores
    vector<double> drops_per_link, flows_per_link;

    chrono::time_point<chrono::high_resolution_clock> timer_checkpoint;
};

#endif
