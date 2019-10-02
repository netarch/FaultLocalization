#ifndef __COMMON_TESTING_SYSTEM__
#define __COMMON_TESTING_SYSTEM__
#include <iostream>
#include <assert.h>
#include "utils.h"
#include "logdata.h"
#include "estimator.h"
#include <chrono>

using namespace std;

void GetPrecisionRecallTrendFile(string filename, double min_start_time_ms,
                                double max_finish_time_ms, double step_ms,
                                Estimator* base_estimator, vector<PDD> &result,
                                int nopenmp_threads);

void GetPrecisionRecallTrendFiles(double min_start_time_ms, double max_finish_time_ms, 
                                  double step_ms, vector<PDD> &result,
                                  Estimator* estimator, int nopenmp_threads);

void GetPrecisionRecallParamsFile(string filename, double min_start_time_ms,
                                double max_finish_time_ms, vector<vector<double> > &params,
                                Estimator* base_estimator, vector<PDD> &result,
                                int nopenmp_threads);

void GetPrecisionRecallParamsFiles(double min_start_time_ms, double max_finish_time_ms, 
                                  vector<vector<double> > &params, vector<PDD> &result,
                                  Estimator* estimator, int nopenmp_threads);
#endif
