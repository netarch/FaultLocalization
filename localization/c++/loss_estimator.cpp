#include "bayesian_net.h"
#include "bayesian_net_continuous.h"
#include "bayesian_net_sherlock.h"
#include "doubleO7.h"
#include "net_bouncer.h"
#include "utils.h"
#include <assert.h>
#include <chrono>
#include <iostream>

using namespace std;
extern bool FLOW_DELAY;

int main(int argc, char *argv[]) {
    VERBOSE = true;
    assert(argc == 6);
    string trace_file(argv[1]);
    cout << "Running analysis on file " << trace_file << endl;
    string topology_file(argv[2]);
    cout << "Reading topology from file " << topology_file << endl;
    double min_start_time_ms = atof(argv[3]) * 1000.0,
           max_finish_time_ms = atof(argv[4]) * 1000.0;
    int nopenmp_threads = atoi(argv[5]);
    cout << "Using " << nopenmp_threads << " openmp threads" << endl;
    // int nchunks = 32;
    LogData data;
    // GetDataFromLogFile(trace_file, &data);

    /*
    DoubleO7 estimator; vector<double> params = {0.0001}; //{0.0019};
    //BayesianNet estimator; vector<double> params = {1.0-7e-3, 4.5e-4, -30.0};
    //A2 PATH_KNOWN = false; TRACEROUTE_BAD_FLOWS = true; INPUT_FLOW_TYPE =
    PROBLEMATIC_FLOWS;
    */

    /*
    BayesianNet estimator; vector<double> params = {1.0-5.0e-3, 1.5e-4, -10.0};
    //A1
    //NetBouncer estimator; vector<double> params = {0.21, 0.001, 0.075};
    PATH_KNOWN = true;
    TRACEROUTE_BAD_FLOWS = false;
    INPUT_FLOW_TYPE = ACTIVE_FLOWS;
    */

    Sherlock estimator;
    vector<double> params = {1.0 - 3.0e-3, 2.0e-4,
                             -20.0}; // for scale experiment
    PATH_KNOWN = false;
    TRACEROUTE_BAD_FLOWS = true;
    INPUT_FLOW_TYPE = ALL_FLOWS;
    /*
     */

    /*
    //NetBouncer estimator; vector<double> params = {0.51, 0.0205, 0.41}; //INT
    //NetBouncer estimator; vector<double> params = {0.3, 0.003, 0.41}; // for
    scale experiment BayesianNet estimator; vector<double> params =
    {1.0-3.0e-3, 2.0e-4, -20.0}; // for scale experiment PATH_KNOWN = true;
    TRACEROUTE_BAD_FLOWS = false;
    INPUT_FLOW_TYPE = ALL_FLOWS;
    */

    // NetBouncer estimator; vector<double> params = {0.06, 0.00225, 0.06};
    // NetBouncer estimator; vector<double> params = {0.31, 0.005, 0.125};
    // //misconfigured_acl BayesianNet estimator; vector<double> params =
    // {1.0-0.002, 0.00045, -10.0}; //INT / A1+A2+P

    // Sherlock estimator; vector<double> params = {1.0-0.004, 0.0001, -15.0};
    // BayesianNet estimator; vector<double> params = {1.0-0.01, 0.0008,
    // -100.0}; MISCONFIGURED_ACL = true; BayesianNet estimator; vector<double>
    // params = {1.0-0.02, 0.00002, -50.0}; //misconfigured_acl, conditional
    // vector<double> params = {1.0-1.0e-3, 1.0e-4, -20.0};
    estimator.SetParams(params);

    // should go after declaring estimator
    GetDataFromLogFileParallel(trace_file, topology_file, &data,
                               nopenmp_threads);
    // cout << "Num flows "<< data.flows.size() << endl;
    Hypothesis failed_links_set;
    data.GetFailedLinkIds(failed_links_set);
    estimator.SetLogData(&data, max_finish_time_ms, nopenmp_threads);

    Hypothesis estimator_hypothesis;
    auto start_localization_time = chrono::high_resolution_clock::now();
    estimator.LocalizeFailures(min_start_time_ms, max_finish_time_ms,
                               estimator_hypothesis, nopenmp_threads);
    PDD precision_recall;
    if (FLOW_DELAY)
        precision_recall = GetPrecisionRecallUnidirectional(
            failed_links_set, estimator_hypothesis, &data);
    else
        precision_recall =
            GetPrecisionRecall(failed_links_set, estimator_hypothesis, &data);
    cout << "Output Hypothesis: " << data.IdsToLinks(estimator_hypothesis)
         << " precsion_recall " << precision_recall.first << " "
         << precision_recall.second << endl;
    cout << "Finished localization in "
         << GetTimeSinceSeconds(start_localization_time) << " seconds" << endl;
    return 0;
}
