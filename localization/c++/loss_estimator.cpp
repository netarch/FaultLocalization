#include <iostream>
#include <assert.h>
#include "utils.h"
#include "bayesian_net.h"
#include <chrono>

using namespace std;

int main(int argc, char *argv[]){
    assert (argc == 5);
    string filename (argv[1]); 
    cout << "Running analysis on file "<<filename << endl;
    double min_start_time_ms = atof(argv[2]) * 1000.0, max_finish_time_ms = atof(argv[3]) * 1000.0;
    int nopenmp_threads = atoi(argv[4]);
    cout << "Using " << nopenmp_threads << " openmp threads"<<endl;
    cout << "sizeof (Flow) " << sizeof(Flow) << " sizeof(vector) " << sizeof(vector<int>) << " sizeof(Path) " << sizeof (Path) << endl;
    cout << "sizeof (int*) " << sizeof(int*) << " sizeof(char) " << sizeof(char) << endl;
    //LogFileData* data = GetDataFromLogFile(filename);
    int nchunks = 32;
    LogFileData* data = GetDataFromLogFileDistributed(filename, nchunks, nchunks);
    Hypothesis failed_links_set;
    data->GetFailedLinkIds(failed_links_set);
    BayesianNet estimator;
    if (estimator.USE_CONDITIONAL){
        data->FilterFlowsForConditional(max_finish_time_ms, nopenmp_threads);
    }
    Hypothesis estimator_hypothesis;
    estimator.LocalizeFailures(data, min_start_time_ms, max_finish_time_ms,
                                        estimator_hypothesis, nopenmp_threads);
    PDD precision_recall = GetPrecisionRecall(failed_links_set, estimator_hypothesis);
    cout << "Output Hypothesis: " << data->IdsToLinks(estimator_hypothesis) << " precsion_recall "
         <<precision_recall.first << " " << precision_recall.second<<endl;
    return 0;
}
