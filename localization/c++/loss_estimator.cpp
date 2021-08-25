#include <iostream>
#include <assert.h>
#include "utils.h"
#include "bayesian_net.h"
#include "doubleO7.h"
#include "net_bouncer.h"
#include "bayesian_net_continuous.h"
#include "bayesian_net_sherlock.h"
#include <chrono>

using namespace std;

int main(int argc, char *argv[]){
    assert (argc == 6);
    string trace_file (argv[1]); 
    cout << "Running analysis on file "<<trace_file << endl;
    string topology_file (argv[2]);
    cout << "Reading topology from file " << topology_file << endl;
    double min_start_time_ms = atof(argv[3]) * 1000.0, max_finish_time_ms = atof(argv[4]) * 1000.0;
    int nopenmp_threads = atoi(argv[5]);
    cout << "Using " << nopenmp_threads << " openmp threads"<<endl;
    //int nchunks = 32;
    LogData data;
    //GetDataFromLogFile(trace_file, &data);
    GetDataFromLogFileParallel(trace_file, topology_file, &data, nopenmp_threads);
    Hypothesis failed_links_set;
    data.GetFailedLinkIds(failed_links_set);
    //DoubleO7 estimator; vector<double> params = {0.0025};
    //NetBouncer estimator; vector<double> params = {0.016, 0.0113};
    //Sherlock estimator;
    BayesianNet estimator; vector<double> params = {1.0-3.0e-3, 2.0e-4, -20.0};
    //vector<double> params = {1.0-1.0e-3, 1.0e-4, -20.0};
    estimator.SetParams(params);
    estimator.SetLogData(&data, max_finish_time_ms, nopenmp_threads);
    Hypothesis estimator_hypothesis;
    auto start_localization_time = chrono::high_resolution_clock::now();
    estimator.LocalizeFailures(min_start_time_ms, max_finish_time_ms,
                               estimator_hypothesis, nopenmp_threads);
    PDD precision_recall = GetPrecisionRecall(failed_links_set, estimator_hypothesis);
    cout << "Output Hypothesis: " << data.IdsToLinks(estimator_hypothesis) << " precsion_recall "
         <<precision_recall.first << " " << precision_recall.second<<endl;
    cout << "Finished localization in "<< GetTimeSinceSeconds(start_localization_time) << " seconds" << endl;
    return 0;
}
