#ifndef __FAULT_LOCALIZE_SHERLOCK__
#define __FAULT_LOCALIZE_SHERLOCK__
#include "bayesian_net.h"

class Sherlock : public BayesianNet{
 public:
    Sherlock() : BayesianNet() {}
    void LocalizeFailures(double min_start_time_ms, double max_finish_time_ms,
                          Hypothesis &localized_links, int nopenmp_threads);
    const int MAX_FAILS = 2;
    const int N_MAX_K_LIKELIHOODS = 20;
    Sherlock* CreateObject();

private:
    void SearchHypothesesFlock(double min_start_time_ms, double max_finish_time_ms,
                          unordered_map<Hypothesis*, double> &all_hypothesis,
                          int nopenmp_threads);
    void SearchHypotheses(double min_start_time_ms, double max_finish_time_ms,
                          unordered_map<Hypothesis*, double> &all_hypothesis,
                          int nopenmp_threads);
    chrono::time_point<chrono::high_resolution_clock> timer_checkpoint;
};

#endif
