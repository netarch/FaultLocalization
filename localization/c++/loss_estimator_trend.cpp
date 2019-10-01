#include <iostream>
#include <assert.h>
#include "utils.h"
#include "bayesian_net.h"
#include <chrono>

using namespace std;

vector<string> GetFiles(){
    string file_prefix = "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/topology/ft_k12_os3/plog";
    vector<PII> ignore_files = {};
    vector<string> files;
    //for(int f=1; f<=8; f++){
    for (int f: vector<int>({1})){ // {1-8}
        for(int s: vector<int>({3})){ // {3}
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" + to_string(s)); 
            }
        }
    }
    return files;
}

void GetPrecisionRecallTrendFile(string filename, double min_start_time_ms,
                                double max_finish_time_ms, double step_ms,
                                vector<PDD> &result, int nopenmp_threads){
    assert (result.size() == 0);
    LogData* data = new LogData();
    GetDataFromLogFile(filename, data);
    Hypothesis failed_links_set;
    data->GetFailedLinkIds(failed_links_set);
    BayesianNet estimator;
    if(estimator.USE_CONDITIONAL){
        data->FilterFlowsForConditional(max_finish_time_ms, nopenmp_threads);
    }
    for(double finish_time_ms=min_start_time_ms+step_ms;
            finish_time_ms<=max_finish_time_ms; finish_time_ms += step_ms){
        auto start_bin_time = chrono::high_resolution_clock::now();
        cout << "Finish time " << finish_time_ms << endl;
        vector<vector<int> >* flows_by_link_id = data->GetFlowsByLinkId(finish_time_ms, nopenmp_threads);
        estimator.SetFlowsByLinkId(flows_by_link_id);
        cout << "Finished binning " << data->flows.size() << " flows by links in "
             << GetTimeSinceMilliSeconds(start_bin_time) << " seconds" << endl;
        Hypothesis estimator_hypothesis;
        estimator.LocalizeFailures(data, min_start_time_ms, finish_time_ms,
                                   estimator_hypothesis, nopenmp_threads);
        PDD precision_recall = GetPrecisionRecall(failed_links_set, estimator_hypothesis);
        result.push_back(precision_recall);
        cout << "Output Hypothesis: " << data->IdsToLinks(estimator_hypothesis)<< " precsion_recall "
             <<precision_recall.first << " " << precision_recall.second<<endl;
        delete(flows_by_link_id);
    }
}

void GetPrecisionRecallTrendFiles(double min_start_time_ms, double max_finish_time_ms, 
                                  double step_ms, vector<PDD> &result, int nopenmp_threads){
    vector<string> filenames = GetFiles();
    result.clear();
    for(double finish_time_ms=min_start_time_ms+step_ms;
            finish_time_ms<=max_finish_time_ms; finish_time_ms += step_ms){
        result.push_back(PDD(0.0, 0.0));
    }
    mutex lock;
    int nfiles = filenames.size();
    int nthreads1 = min({4, nfiles, nopenmp_threads});
    int nthreads2 = nopenmp_threads/nthreads1;
    cout << nthreads1 << " " << nthreads2 << endl;
    #pragma omp parallel for num_threads(nthreads1)
    for (int ff=0; ff<filenames.size(); ff++){
        string filename = filenames[ff];
        vector<PDD> intermediate_result;
        GetPrecisionRecallTrendFile(filename, min_start_time_ms, max_finish_time_ms, step_ms,
                                    intermediate_result, nthreads2);
        assert(intermediate_result.size() == result.size()); 
        lock.lock();
        for (int i=0; i<intermediate_result.size(); i++)
            result[i] = result[i] + intermediate_result[i];
        lock.unlock();
    }
    for (int i=0; i<result.size(); i++){
        auto [p, r] = result[i];
        result[i] = PDD(p/nfiles, r/nfiles);
    }
}

int main(int argc, char *argv[]){
    assert (argc == 5);
    double min_start_time_ms = atof(argv[1]) * 1000.0, max_finish_time_ms = atof(argv[2]) * 1000.0;
    double step_ms = atof(argv[3]) * 1000.0;
    int nopenmp_threads = atoi(argv[4]);
    cout << "Using " << nopenmp_threads << " openmp threads"<<endl;
    vector<PDD> result;
    GetPrecisionRecallTrendFiles(min_start_time_ms, max_finish_time_ms, step_ms, result, nopenmp_threads);
    int ctr = 0;
    for(double finish_time_ms=min_start_time_ms+step_ms;
            finish_time_ms<=max_finish_time_ms; finish_time_ms += step_ms){
        cout << "Finish time (ms) " << finish_time_ms << " " << result[ctr++] << endl;
    }
    //string filename (argv[1]); 
    //cout << "Running analysis on file "<<filename << endl;
    return 0;
}
