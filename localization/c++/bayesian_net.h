#ifndef __FAULT_LOCALIZE_BAYESIAN_NET__
#define __FAULT_LOCALIZE_BAYESIAN_NET__
#include "estimator.h"

class BayesianNet : public Estimator{
 public:
    BayesianNetEstimator(){}
    Hypothesis* LocalizeFailures(LogFileData* data);
    const int MAX_FAILS = 10;
    // For printing purposes
    const int N_MAX_K_LIKELIHOODS = 20;
    const bool USE_CONDITIONAL = false;

private:
    double ComputeLogLikelihood(set<Link>* hypothesis, double min_start_time_ms, double max_finish_time_ms);
    double ComputeLogLikelihoodConditional(set<Link>* hypothesis, double min_start_time_ms, double max_finish_time_ms);


    // Noise parameters
    double p1, p2;
    LogFileData* data_cache;
};

#endif
