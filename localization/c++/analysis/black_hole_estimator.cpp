#include "bayesian_net.h"
#include "bayesian_net_continuous.h"
#include "bayesian_net_sherlock.h"
#include "doubleO7.h"
#include "net_bouncer.h"
#include "black_hole_utils.h"
#include "utils.h"
#include <assert.h>
#include <bits/stdc++.h>
#include <chrono>
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
    assert(argc == 6);
    string trace_file(argv[1]);
    cout << "Running analysis on file " << trace_file << endl;
    string topology_file(argv[2]);
    cout << "Reading topology from file " << topology_file << endl;
    double min_start_time_ms = atof(argv[3]) * 1000.0,
           max_finish_time_ms = atof(argv[4]) * 1000.0;
    int nopenmp_threads = atoi(argv[5]);
    cout << "Using " << nopenmp_threads << " openmp threads" << endl;

    BayesianNet estimator;
    vector<double> params = {1.0 - 1.0e-3, 1.0e-4, -25.0};
    estimator.SetParams(params);
    PATH_KNOWN = false;
    TRACEROUTE_BAD_FLOWS = false;
    INPUT_FLOW_TYPE = APPLICATION_FLOWS;

    LogData data;
    cout << "calling GetDataFromLogFileParallel" << endl;
    GetDataFromLogFileParallel(trace_file, topology_file, &data,
                               nopenmp_threads);
    string fail_file = trace_file + ".fails";
    auto failed_components = ReadFailuresBlackHole(fail_file);

    LocalizeScore(data, max_finish_time_ms);
    // LocalizeProbAnalysis(data, failed_components, min_start_time_ms,
    // max_finish_time_ms, nopenmp_threads);
    return 0;
}
