#include <iostream>
#include <assert.h>
#include "utils.h"
#include "logdata.h"
#include "bayesian_net.h"
#include "common_testing_system.h"
#include "reduced_utils.h"
#include <chrono>

using namespace std;

vector<string> GetFiles();

std::atomic<int> total_flows{0};

string GetFileNameFromPath(string file_path){
    int pos = file_path.find_last_of('/');
    if (pos == string::npos) pos = -1;
    return file_path.substr(pos+1);
}

double GetPrecisionRecallTrendFile(string topology_file, string trace_file, double min_start_time_ms,
                                double max_finish_time_ms, double step_ms,
                                Estimator* base_estimator, vector<PDD> &result,
                                int nopenmp_threads){
    assert (result.size() == 0);
    LogData* data = new LogData();
    GetDataFromLogFileParallel(trace_file, topology_file, data, nopenmp_threads);
    data->FilterFlowsBeforeTime(max_finish_time_ms, nopenmp_threads);
    auto start_analysis_time = chrono::high_resolution_clock::now();
    Hypothesis failed_links_set;
    data->GetFailedLinkIds(failed_links_set);
    Estimator* estimator = base_estimator->CreateObject();
    estimator->SetLogData(data, max_finish_time_ms, nopenmp_threads);
    for(double finish_time_ms=min_start_time_ms+step_ms;
            finish_time_ms<=max_finish_time_ms; finish_time_ms += step_ms){
        Hypothesis estimator_hypothesis;
        cout << "Flows " << data->flows.size() << " " << min_start_time_ms << " " << finish_time_ms << " " << topology_file << " " << nopenmp_threads << endl;
        estimator->LocalizeFailures(min_start_time_ms, finish_time_ms,
                                   estimator_hypothesis, nopenmp_threads);
        PDD precision_recall = GetPrecisionRecall(failed_links_set, estimator_hypothesis, data);
        result.push_back(precision_recall);
        total_flows += data->flows.size();
        cout << GetFileNameFromPath(trace_file) << " Flows " << data->flows.size()
             << " Finish time " << finish_time_ms << " Output Hypothesis: "
             << data->IdsToLinks(estimator_hypothesis)<< " precsion_recall "
             << precision_recall.first << " " << precision_recall.second<<endl;
    }
    delete(estimator);
    return GetTimeSinceSeconds(start_analysis_time);
}

void GetPrecisionRecallTrendFiles(string topology_file, double min_start_time_ms, double max_finish_time_ms, 
                                  double step_ms, vector<PDD> &result,
                                  Estimator* estimator, int nopenmp_threads){
    vector<string> trace_files = GetFiles();
    result.clear();
    for(double finish_time_ms=min_start_time_ms+step_ms;
            finish_time_ms<=max_finish_time_ms; finish_time_ms += step_ms){
        result.push_back(PDD(0.0, 0.0));
    }
    mutex lock;
    int nfiles = trace_files.size();
    int nthreads1 = min({32, nfiles, nopenmp_threads});
    int nthreads2 = 1;
    //int nthreads1 = 1;
    //int nthreads2 = nopenmp_threads;
    cout << nthreads1 << " " << nthreads2 << endl;
    double avg_analysis_time_sec = 0.0;
    #pragma omp parallel for num_threads(nthreads1) if (nthreads1 > 1)
    for (int ff=0; ff<trace_files.size(); ff++){
        string trace_file = trace_files[ff];
        vector<PDD> intermediate_result;
        double analysis_time_sec = GetPrecisionRecallTrendFile(topology_file, trace_file, min_start_time_ms,
                                     max_finish_time_ms, step_ms, estimator, intermediate_result, nthreads2);
        assert(intermediate_result.size() == result.size()); 
        lock.lock();
        avg_analysis_time_sec += analysis_time_sec;
        for (int i=0; i<intermediate_result.size(); i++)
            result[i] = result[i] + intermediate_result[i];
        lock.unlock();
    }
    for (int i=0; i<result.size(); i++){
        auto [p, r] = result[i];
        result[i] = PDD(p/nfiles, r/nfiles);
    }
    avg_analysis_time_sec /= trace_files.size();
    cout << "Average flows "<< total_flows/trace_files.size() << " avg-time-for-analysis "<< avg_analysis_time_sec << " seconds" << endl;
}

void GetPrecisionRecallParamsFile(string topology_file, string trace_file, double min_start_time_ms,
                                double max_finish_time_ms, vector<vector<double> > &params,
                                Estimator* base_estimator, vector<PDD> &result,
                                int nopenmp_threads){
    assert (result.size() == 0);
    LogData *data = new LogData();
    GetDataFromLogFileParallel(trace_file, topology_file, data, nopenmp_threads);

    Estimator* estimator = base_estimator->CreateObject();

    const bool REDUCED_ANALYSIS  = false;
    if constexpr (REDUCED_ANALYSIS) {
        BayesianNet* b_estimator = static_cast<BayesianNet*>(estimator);
        string reduced_map_file = "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/ns3/topology/ft_k10_os3/ns3ft_deg10_sw125_svr250_os3_i1.reduced";
        LogData *reduced_data = new LogData();
        SetInputForReduced(*b_estimator, data, reduced_data, reduced_map_file, max_finish_time_ms, nopenmp_threads);
        data = reduced_data;
    }

    Hypothesis failed_links_set;
    data->GetFailedLinkIds(failed_links_set);
    estimator->SetLogData(data, max_finish_time_ms, nopenmp_threads);
    for(int ii=0; ii<params.size(); ii++){
        estimator->SetParams(params[ii]);
        Hypothesis estimator_hypothesis;
        estimator->LocalizeFailures(min_start_time_ms, max_finish_time_ms,
                                   estimator_hypothesis, nopenmp_threads);
        PDD precision_recall = GetPrecisionRecall(failed_links_set, estimator_hypothesis, data);
        result.push_back(precision_recall);
        if (VERBOSE or true) {
            //cout << "Failed links " << failed_links_set << " predicted: " << estimator_hypothesis << " ";
            cout << GetFileNameFromPath(trace_file) << " flows " << data->flows.size() << " "
                 << params[ii] << " " << data->IdsToLinks(estimator_hypothesis)<< "  "
                 << precision_recall.first << " " << precision_recall.second << endl;
        }
    }
    delete(data);
    delete(estimator);
}

void GetPrecisionRecallParamsFiles(string topology_file, double min_start_time_ms, double max_finish_time_ms, 
                                  vector<vector<double> > &params, vector<PDD> &result,
                                  Estimator* estimator, int nopenmp_threads){
    vector<string> trace_files = GetFiles();
    result.clear();
    for (int ii=0; ii<params.size(); ii++){
        result.push_back(PDD(0.0, 0.0));
    }
    mutex lock;
    int nfiles = trace_files.size();
    int nthreads1 = min({32, nfiles, nopenmp_threads});
    int nthreads2 = 1;
    //int nthreads1 = 1, nthreads2 = nopenmp_threads;
    cout << nthreads1 << " " << nthreads2 << endl;
    #pragma omp parallel for num_threads(nthreads1)
    for (int ff=0; ff<trace_files.size(); ff++){
        string trace_file = trace_files[ff];
        vector<PDD> intermediate_result;
        GetPrecisionRecallParamsFile(topology_file, trace_file, min_start_time_ms, max_finish_time_ms, params,
                                    estimator, intermediate_result, nthreads2);
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
