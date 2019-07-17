#ifndef FAULT_LOCALIZE_ESTIMATOR
#define FAULT_LOCALIZE_ESTIMATOR

#include "flow.h"
#include "link.h"

struct EstimatorResult{
    double precision;
    double recall;
    void *info;
    EstimatorResult(double _precision, double _recall, void* _info==NULL):
        precision(_precision), recall(_recall), info(_info){}
};


class Estimator{
 public:
    Estimator(){}

    vector<Link>* localize(vector<Flow> &flows, vector<Link> &links);

};

#endif
