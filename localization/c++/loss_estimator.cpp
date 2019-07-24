#include <iostream>
#include <assert.h>
#include "utils.h"
#include "estimator.h"
#include <chrono>

using namespace std;

int main(int argc, char *argv[]){
    assert (argc == 4);
    string filename (argv[1]); 
    cout << "Running analysis on file "<<filename << endl;
    double min_start_time_ms = atof(argv[2]), max_finish_time_ms = atof(argv[3]);
    LogFileData* data = GetDataFromLogFile(filename);
    set<Link> failed_links_set;
    data->GetFailedLinksSet(failed_links_set);
    Estimator estimator;
    Hypothesis* estimator_hypothesis = estimator.Localize(data);
    PDD precision_recall = GetPrecisionRecall(failed_links_set, *estimator_hypothesis);
    cout << "Precision "<<precision_recall.first << ", Recall "<<precision_recall.second<<endl;
    return 0;
}
