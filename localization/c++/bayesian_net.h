#ifndef __FAULT_LOCALIZE_BAYESIAN_NET__
#define __FAULT_LOCALIZE_BAYESIAN_NET__
#include "estimator.h"

class BayesianNet : public Estimator{
 public:
    BayesianNet() : Estimator() {}
    void LocalizeFailures(LogFileData* data, double min_start_time_ms,
                                 double max_finish_time_ms, Hypothesis &localized_links,
                                 int nopenmp_threads);
    const int MAX_FAILS = 10;
    const int NUM_CANDIDATES = max(15, 5 * MAX_FAILS);
    const int NUM_TOP_HYPOTHESIS_AT_EACH_STAGE = 5;
    // For printing purposes
    const int N_MAX_K_LIKELIHOODS = 20;
    const bool USE_CONDITIONAL = false;
    const double PRIOR = -10.0;

private:
    void ComputeSingleLinkLogLikelihood(vector<pair<double, Hypothesis*> > &result,
                    double min_start_time_ms, double  max_finish_time_ms,
                    int nopenmp_threads);

    void GetRelevantFlows(Hypothesis* hypothesis, Hypothesis* base_hypothesis,
                                   double min_start_time_ms, double max_finish_time_ms,
                                   vector<int>& relevant_flows);

    double ComputeLogLikelihood(Hypothesis* hypothesis, Hypothesis* base_hypothesis,
                    double base_likelihood, double min_start_time_ms,
                    double max_finish_time_ms, vector<int> &relevant_flows);

    double ComputeLogLikelihood(Hypothesis* hypothesis, Hypothesis* base_hypothesis,
                    double base_likelihood, double min_start_time_ms,
                    double max_finish_time_ms);

    void ComputeLogLikelihood(vector<Hypothesis*> &hypothesis_space,
                    vector<pair<Hypothesis*, double> > &base_hypothesis_likelihood,
                    vector<pair<double, Hypothesis*> > &result,
                    double min_start_time_ms, double  max_finish_time_ms,
                    int nopenmp_threads);

    double ComputeLogLikelihoodConditional(set<Link>* hypothesis,
                    double min_start_time_ms, double max_finish_time_ms);

    inline double BnfWeighted(int naffected, int npaths, int naffected_r,
                    int npaths_r, double weight_good, double weight_bad);

    inline double BnfWeightedConditional(int naffected, int npaths, int naffected_r,
                    int npaths_r, double weight_good, double weight_bad);

    // Noise parameters
    double p1 = 1.0-1.0e-3, p2 = 2.5e-4;
    //double p1 = 1.0 - 0.5e-3, p2 = 1.0e-4;
    LogFileData* data_cache;
    vector<vector<int> >* flows_by_link_id_cache;
    //double function1_time_sec[100];
};

/*
 * Perform an atomic addition to the double via spin-locking
 * on compare_exchange_weak. Memory ordering is release on write
 * consume on read
 * Inspired from: https://www.reddit.com/r/cpp/comments/338pcj/atomic_addition_of_floats_using_compare_exchange/
 */
inline double atomic_add_double(atomic<double> &d, double val);

#endif
