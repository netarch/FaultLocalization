#ifndef __FAULT_LOCALIZE_BAYESIAN_NET_CONTINUOUS_
#define __FAULT_LOCALIZE_BAYESIAN_NET_CONTINUOUS_
#include "estimator.h"

class BayesianNetContinuous : public Estimator {
  public:
    BayesianNetContinuous() : Estimator() {}
    void LocalizeFailures(double min_start_time_ms, double max_finish_time_ms,
                          Hypothesis &localized_links, int nopenmp_threads);

    BayesianNetContinuous *CreateObject();
    void SetLogData(LogData *data, double max_finish_time_ms,
                    int nopenmp_threads);
    void SetParams(vector<double> &param);

    const bool APPLY_PRIOR = false;

  private:
    int ComputeGradients(vector<double> &gradients, vector<double> &loss_rates,
                         double start_time_ms, double max_finish_time_ms,
                         int nopenmp_threads);

    inline bool DiscardFlow(Flow *flow, double min_start_time_ms,
                            double max_finish_time_ms);

    long double ComputeLogLikelihood(vector<double> &loss_rates,
                                     double start_time_ms,
                                     double max_finish_time_ms,
                                     int nopenmp_threads);

    inline double PathFailRate(int first_link_id, int last_link_id, Path *path,
                               vector<double> &loss_rates);

    inline long double PathGradientNumerator(int weight_good, int weight_bad,
                                             double path_fail_rate);

    inline long double FlowProbabilityForPath(int weight_good, int weight_bad,
                                              double path_fail_rate);
    double initial_loss_rate = 1.0e-4;
    double learning_rate = 5e-8;
    double moving_average_weight = 0.95;
    double nav_momentum = 0.9;
    double decay_rate = 0.1;

    /* For prior computations */
    inline long double ComputeLogPrior(double loss_rate);
    inline long double ComputeLogPriorGradient(double loss_rate);
    double loss_rate_mid = 1.0e-4;
    double sigmoid_constant = 1.0e4;
};
#endif
