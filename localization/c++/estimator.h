#ifndef __FAULT_LOCALIZE_ESTIMATOR__
#define __FAULT_LOCALIZE_ESTIMATOR__

#include "flow.h"
#include "utils.h"
#include <set>

struct EstimatorResult{
    double precision;
    double recall;
    void *info;
    EstimatorResult(double _precision, double _recall, void* _info=NULL):
        precision(_precision), recall(_recall), info(_info){}
};

class Estimator{
 public:
    Estimator(){}

    virtual Hypothesis* LocalizeFailures(LogFileData* data, double start_time_ms, double max_finish_time_ms){
        return new Hypothesis();
    }
};

#endif
