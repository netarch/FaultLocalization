#ifndef __FAULT_LOCALIZE_BAYESIAN_NET__
#define __FAULT_LOCALIZE_BAYESIAN_NET__
#include "estimator.h"

class BayesianNet : public Estimator{
 public:
    BayesianNet() : Estimator() {}
    Hypothesis* LocalizeFailures(LogFileData* data, double min_start_time_ms,
                                 double max_finish_time_ms);
    const int MAX_FAILS = 10;
    const int NUM_CANDIDATES = max(15, 5 * MAX_FAILS);
    const int NUM_TOP_HYPOTHESIS_AT_EACH_STAGE = 5;
    // For printing purposes
    const int N_MAX_K_LIKELIHOODS = 20;
    const bool USE_CONDITIONAL = false;
    const double PRIOR = -10.0;

private:
    double ComputeLogLikelihood(Hypothesis* hypothesis,
                    Hypothesis* base_hypothesis, double base_likelihood,
                    double min_start_time_ms, double max_finish_time_ms);

    vector<pair<double, Hypothesis*> >* ComputeLogLikelihood(
                    vector<Hypothesis*> &hypothesis_space,
                    vector<pair<Hypothesis*, double> > &base_hypothesis_likelihood,
                    double min_start_time_ms, double  max_finish_time_ms);

    double ComputeLogLikelihoodConditional(set<Link>* hypothesis,
                    double min_start_time_ms, double max_finish_time_ms);

    inline double BnfWeighted(int naffected, int npaths, int naffected_r,
                    int npaths_r, double weight_good, double weight_bad);

    // Noise parameters
    double p1 = 1.0-2.5e-3, p2 = 2.5e-4;
    LogFileData* data_cache;
    unordered_map<Link, vector<int> >* flows_by_link_cache;

    double function1_time_sec = 0.0;
};

#endif
