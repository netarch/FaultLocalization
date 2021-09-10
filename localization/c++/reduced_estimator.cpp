#include <iostream>
#include <assert.h>
#include "utils.h"
#include "bayesian_net.h"
#include "reduced_utils.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>

using namespace std;

int main(int argc, char *argv[]){
    assert (argc == 7);
    string trace_file (argv[1]); 
    string topology_file (argv[2]);
    string reduced_map_file (argv[3]);
    double min_start_time_ms = atof(argv[4]) * 1000.0, max_finish_time_ms = atof(argv[5]) * 1000.0;
    int nopenmp_threads = atoi(argv[6]);
    cout << "Running analysis on file "<< trace_file << endl;
    cout << "Using topology file "<< topology_file <<endl;
    cout << "Using reduced map file "<< reduced_map_file <<endl;
    cout << "Using " << nopenmp_threads << " openmp threads"<<endl;

    BayesianNet estimator;
    LogData data;
    GetDataFromLogFileParallel(trace_file, topology_file, &data, nopenmp_threads);
    if (INPUT_FLOW_TYPE==PROBLEMATIC_FLOWS){
        data.FilterFlowsForConditional(max_finish_time_ms, nopenmp_threads);
    }
    LogData reduced_data;
    SetInputForReduced(estimator, &data, &reduced_data, reduced_map_file, max_finish_time_ms, nopenmp_threads);
    Hypothesis estimator_hypothesis;
    vector<double> params = {1.0-5.0e-3, 2.0e-4, -25.0};
    estimator.SetParams(params);
    estimator.LocalizeFailures(min_start_time_ms, max_finish_time_ms,
                               estimator_hypothesis, nopenmp_threads);
    Hypothesis failed_links_set;
    reduced_data.GetFailedLinkIds(failed_links_set);
    PDD precision_recall = GetPrecisionRecall(failed_links_set, estimator_hypothesis, &reduced_data);
    cout << "Output Hypothesis: " << reduced_data.IdsToLinks(estimator_hypothesis) << " precsion_recall "
         << precision_recall.first << " " << precision_recall.second<<endl;
    return 0;
}
