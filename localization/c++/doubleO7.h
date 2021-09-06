#ifndef __FAULT_LOCALIZE_007__
#define __FAULT_LOCALIZE_007__
#include "estimator.h"

const bool USE_BUGGY_007 = false; //to match matlab script

class DoubleO7: public Estimator{
 public:
    DoubleO7();
    void LocalizeFailures(double min_start_time_ms, double max_finish_time_ms,
                          Hypothesis &localized_components, int nopenmp_threads);

    void LocalizeLinkFailures(double min_start_time_ms, double max_finish_time_ms,
                              Hypothesis &localized_links, int nopenmp_threads);

    void LocalizeDeviceFailures(double min_start_time_ms, double max_finish_time_ms,
                                Hypothesis &localized_device_links, int nopenmp_threads);

    void SetLogData(LogData* data_, double max_finish_time_ms, int nopenmp_threads);
    DoubleO7* CreateObject();
    void SetParams(vector<double>& params);

private:

    PDD ComputeVotesDevice(vector<Flow*>& bad_flows, vector<double>& votes,
                           Hypothesis &problematic_devices, double min_start_time_ms,
                           double max_finish_time_ms);

    PDD ComputeVotes(vector<Flow*>& bad_flows, vector<double>& votes,
                     Hypothesis &problematic_link_ids, double min_start_time_ms,
                     double max_finish_time_ms);

    // Noise parameters
    double fail_threshold = 0.0025;
    const bool ADJUST_VOTES = true;
    const bool DEVICE_ANALYSIS = false;
    const bool FILTER_NOISY_DROPS = false;
};

#endif
