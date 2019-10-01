#ifndef __FAULT_LOCALIZE_ESTIMATOR__
#define __FAULT_LOCALIZE_ESTIMATOR__

#include "flow.h"
#include "utils.h"
#include "logdata.h"
#include <set>

class Estimator{
 public:
    Estimator(){}
    virtual void LocalizeFailures(LogData* data, double min_start_time_ms,
                                 double max_finish_time_ms, Hypothesis &localized_links,
                                 int nopenmp_threads) {}
};

#endif
