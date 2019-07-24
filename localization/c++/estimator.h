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

    set<Link>* localize(LogFileData* data){
        return new set<Link>();
    }
};

#endif
