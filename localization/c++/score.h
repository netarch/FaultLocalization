#ifndef __FAULT_LOCALIZE_SCORE__
#define __FAULT_LOCALIZE_SCORE__
#include "estimator.h"

/* An adaptation of non-probabilistic graphical models like MaxCoverage, 007 and Gestalt */
class Score: public Estimator{
 public:
    Score(): Estimator() {}
    void LocalizeFailures(double min_start_time_ms, double max_finish_time_ms,
                          Hypothesis &localized_links, int nopenmp_threads);

    void SetLogData(LogData* data_, double max_finish_time_ms, int nopenmp_threads);
    Score* CreateObject();
    void SetParams(vector<double>& params);

private:

    PDD ComputeScores(vector<Flow*>& bad_flows, vector<double>& scores,
                     Hypothesis &problematic_link_ids, double min_start_time_ms,
                     double max_finish_time_ms);

    PDD ComputeScoresPassive(vector<double>& scores, Hypothesis &problematic_link_ids,
                           double min_start_time_ms, double max_finish_time_ms);
    bool USE_PASSIVE = false;
    bool USE_ABSOLUTE_THRESHOLD = false;
    const bool ADJUST_VOTES = true;
    // Noise parameters
    double fail_threshold = 0.0025;
};

#endif
