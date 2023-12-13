#ifndef __FAULT_LOCALIZE_NET_BOUNCER__
#define __FAULT_LOCALIZE_NET_BOUNCER__
#include "estimator.h"

/*
    The NetBouncer inference algorithm as described in
    "NetBouncer: Active Device and Link Failure Localization in
     Data Center Networks", NSDI 2019
*/

class NetBouncer : public Estimator {
  public:
    NetBouncer();
    void LocalizeFailures(double min_start_time_ms, double max_finish_time_ms,
                          Hypothesis &localized_links, int nopenmp_threads);

    void SetLogData(LogData *data_, double max_finish_time_ms,
                    int nopenmp_threads);
    NetBouncer *CreateObject();
    void SetParams(vector<double> &params);

  private:
    // Get estimated success rate of a flow (whose path is known)
    inline double EstimatedPathSuccessRate(Flow *flow,
                                           vector<double> &success_prob);

    // Remove faulty devices before localizing link failures
    void RemoveBadDevices(double min_start_time_ms, double max_finish_time_ms,
                          int nopenmp_threads);

    void DetectBadDevices(vector<int> &bad_devices, double min_start_time_ms,
                          double max_finish_time_ms, int nopenmp_threads);

    /*
       Compute error between
       (i) estimated flow drop rates based on current link success rates
       estimates and (ii) the observed drop rates of flows
    */
    double ComputeError(vector<Flow *> &active_flows,
                        vector<double> &success_prob, double min_start_time_ms,
                        double max_finish_time_ms);

    /*
       Compute the optimal success rate of var_link_id that will minimize the
       error, assuming success rates of all other links to be constant.
       Used for the coordinate descent algorithm
    */
    double ArgMinError(vector<double> &success_prob, int var_link_id,
                       double min_start_time_ms, double max_finish_time_ms);

    double regularize_const = 0.01;
    double fail_threshold = 0.001;

    // The original NetBouncer set this to 1.0,
    // we calibrate it as any other parameter for optimal results
    double bad_device_frac_bad_flows_threshold = 0.2;

    const int MAX_ITERATIONS = 100;

    // If enabled, each flow is weighted by its size in the error function
    bool USE_MULTIPLIER = false;

    vector<bool> bad_device_links;
    unordered_set<int> bad_devices;
};
#endif
