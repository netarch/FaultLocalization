#include <iostream>
#include <assert.h>
#include "utils.h"
#include "logdata.h"
#include "common_testing_system.h"
#include <chrono>

using namespace std;

vector<string> GetFiles();

void GetPrecisionRecallTrendFile(string filename, double min_start_time_ms,
                                double max_finish_time_ms, double step_ms,
                                Estimator* base_estimator, vector<PDD> &result,
                                int nopenmp_threads){
    assert (result.size() == 0);
    LogData* data = new LogData();
    GetDataFromLogFile(filename, data);
    Hypothesis failed_links_set;
    data->GetFailedLinkIds(failed_links_set);
    Estimator* estimator = base_estimator->CreateObject();
    estimator->SetLogData(data, max_finish_time_ms, nopenmp_threads);
    for(double finish_time_ms=min_start_time_ms+step_ms;
            finish_time_ms<=max_finish_time_ms; finish_time_ms += step_ms){
        Hypothesis estimator_hypothesis;
        estimator->LocalizeFailures(min_start_time_ms, finish_time_ms,
                                   estimator_hypothesis, nopenmp_threads);
        PDD precision_recall = GetPrecisionRecall(failed_links_set, estimator_hypothesis);
        result.push_back(precision_recall);
        cout << "Finish time " << finish_time_ms << " Output Hypothesis: "
             << data->IdsToLinks(estimator_hypothesis)<< " precsion_recall "
             << precision_recall.first << " " << precision_recall.second<<endl;
    }
    delete(estimator);
}

void GetPrecisionRecallTrendFiles(double min_start_time_ms, double max_finish_time_ms, 
                                  double step_ms, vector<PDD> &result,
                                  Estimator* estimator, int nopenmp_threads){
    vector<string> filenames = GetFiles();
    result.clear();
    for(double finish_time_ms=min_start_time_ms+step_ms;
            finish_time_ms<=max_finish_time_ms; finish_time_ms += step_ms){
        result.push_back(PDD(0.0, 0.0));
    }
    mutex lock;
    int nfiles = filenames.size();
    int nthreads1 = min({8, nfiles, nopenmp_threads});
    int nthreads2 = nopenmp_threads/nthreads1;
    cout << nthreads1 << " " << nthreads2 << endl;
    #pragma omp parallel for num_threads(nthreads1)
    for (int ff=0; ff<filenames.size(); ff++){
        string filename = filenames[ff];
        vector<PDD> intermediate_result;
        GetPrecisionRecallTrendFile(filename, min_start_time_ms, max_finish_time_ms, step_ms,
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

void GetPrecisionRecallParamsFile(string filename, double min_start_time_ms,
                                double max_finish_time_ms, vector<vector<double> > &params,
                                Estimator* base_estimator, vector<PDD> &result,
                                int nopenmp_threads){
    assert (result.size() == 0);
    LogData* data = new LogData();
    GetDataFromLogFile(filename, data);
    Hypothesis failed_links_set;
    data->GetFailedLinkIds(failed_links_set);
    Estimator* estimator = base_estimator->CreateObject();
    estimator->SetLogData(data, max_finish_time_ms, nopenmp_threads);
    for(int ii=0; ii<params.size(); ii++){
        estimator->SetParams(params[ii]);
        Hypothesis estimator_hypothesis;
        estimator->LocalizeFailures(min_start_time_ms, max_finish_time_ms,
                                   estimator_hypothesis, nopenmp_threads);
        PDD precision_recall = GetPrecisionRecall(failed_links_set, estimator_hypothesis);
        result.push_back(precision_recall);
        if constexpr (VERBOSE or true) {
            cout << filename << " " << params[ii] << " "
                << data->IdsToLinks(estimator_hypothesis)<< "  "
                << precision_recall.first << " " << precision_recall.second<<endl;
        }
    }
    delete(estimator);
}

void GetPrecisionRecallParamsFiles(double min_start_time_ms, double max_finish_time_ms, 
                                  vector<vector<double> > &params, vector<PDD> &result,
                                  Estimator* estimator, int nopenmp_threads){
    vector<string> filenames = GetFiles();
    result.clear();
    for (int ii=0; ii<params.size(); ii++){
        result.push_back(PDD(0.0, 0.0));
    }
    mutex lock;
    int nfiles = filenames.size();
    int nthreads1 = min({1, nfiles, nopenmp_threads});
    int nthreads2 = nopenmp_threads/nthreads1;
    cout << nthreads1 << " " << nthreads2 << endl;
    //#pragma omp parallel for num_threads(nthreads1) if (nthreads1 > 1)
    for (int ff=0; ff<filenames.size(); ff++){
        string filename = filenames[ff];
        vector<PDD> intermediate_result;
        GetPrecisionRecallParamsFile(filename, min_start_time_ms, max_finish_time_ms, params,
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
