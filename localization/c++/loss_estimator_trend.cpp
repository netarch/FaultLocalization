#include <iostream>
#include <assert.h>
#include "utils.h"
#include "common_testing_system.h"
#include <chrono>
#include "bayesian_net.h"
#include "doubleO7.h"
#include "score.h"

using namespace std;

vector<string> GetFilesNormal(){
    string file_prefix = "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/topology/ft_k12_os3/plog";
    vector<PII> ignore_files = {{4,3}};
    vector<string> files;
    for(int f=1; f<=8; f++){
    //for (int f: vector<int>({8})){ // {1-8}
        for(int s: vector<int>({3,4})){ // {1-4}
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" + to_string(s)); 
            }
        }
    }
    return files;
}

vector<string> GetFilesFlowSimulator(){
    string file_prefix = "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/localization/flow_simulator/ft_logs/plog";
    vector<pair<string, int> > ignore_files = {};
    vector<string> files;
    //vector<string> loss_rate_strings = {"0.001", "0.002", "0.004", "0.006", "0.008"};
    vector<string> loss_rate_strings = {"0.002"};
    for (string& loss_rate_string: loss_rate_strings){
        for(int i=1; i<=1; i++){
            for(int f: {10}){
                if(find(ignore_files.begin(), ignore_files.end(),  pair<string, int>(loss_rate_string, i)) == ignore_files.end()){
                    files.push_back(file_prefix + "_" + loss_rate_string + "_" + to_string(f) + "_" + to_string(i)); 
                    cout << "adding file for analaysis " <<files.back() << endl;
                }
            }
        }
    }
    return files;
}
vector<string> GetFilesSoftness(){
    string file_prefix = "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/topology/ft_k12_os3/softness_logs/plog";
    vector<pair<string, int> > ignore_files = {};
    vector<string> files;
    //vector<string> loss_rate_strings = {"0.001", "0.002", "0.004", "0.006", "0.008"};
    vector<string> loss_rate_strings = {"0.001"};
    for (string& loss_rate_string: loss_rate_strings){
        for(int i=1; i<=16; i++){
            if(find(ignore_files.begin(), ignore_files.end(),  pair<string, int>(loss_rate_string, i)) == ignore_files.end()){
                files.push_back(file_prefix + "_1_" + loss_rate_string + "_" + to_string(i)); 
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    return files;
}

vector<string> GetFiles007Verification(){
    string file_prefix = "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/topology/ft_core10_pods2_agg8_tor20_hosts40/fail_network_links/plog";
    vector<pair<string, int> > ignore_files = {};
    vector<string> files;
    string loss_rate_string = "0.0001";
    for(int i=1; i<=16; i++){
        if(find(ignore_files.begin(), ignore_files.end(),  pair<string, int>(loss_rate_string, i)) == ignore_files.end()){
            files.push_back(file_prefix + "_1_" + loss_rate_string + "_" + to_string(i)); 
            cout << "adding file for analaysis " <<files.back() << endl;
        }
    }
    return files;
}

vector<string> GetFiles(){
    //return GetFilesNormal();
    //return GetFiles007Verification();
    return GetFilesFlowSimulator();
    //return GetFilesSoftness();
}

void GetPrecisionRecallTrendScore(double min_start_time_ms, double max_finish_time_ms,
                                double step_ms, int nopenmp_threads){
    vector<PDD> result;
    Score estimator;
    vector<double> param = {1.0};
    estimator.SetParams(param);
    GetPrecisionRecallTrendFiles(min_start_time_ms, max_finish_time_ms, step_ms,
                                 result, &estimator, nopenmp_threads);
    int ctr = 0;
    for(double finish_time_ms=min_start_time_ms+step_ms;
            finish_time_ms<=max_finish_time_ms; finish_time_ms += step_ms){
        cout << "Finish time (ms) " << finish_time_ms << " " << result[ctr++] << endl;
    }
}

void GetPrecisionRecallTrend007(double min_start_time_ms, double max_finish_time_ms,
                                double step_ms, int nopenmp_threads){
    vector<PDD> result;
    DoubleO7 estimator;
    vector<double> param = {0.01};
    estimator.SetParams(param);
    GetPrecisionRecallTrendFiles(min_start_time_ms, max_finish_time_ms, step_ms,
                                 result, &estimator, nopenmp_threads);
    int ctr = 0;
    for(double finish_time_ms=min_start_time_ms+step_ms;
            finish_time_ms<=max_finish_time_ms; finish_time_ms += step_ms){
        cout << "Finish time (ms) " << finish_time_ms << " " << result[ctr++] << endl;
    }
}

void GetPrecisionRecallTrendBayesianNet(double min_start_time_ms, double max_finish_time_ms,
                                        double step_ms, int nopenmp_threads){
    vector<PDD> result;
    BayesianNet estimator;
    vector<double> param = {1.0-4.0e-3, 1.5e-4};
    estimator.SetParams(param);
    GetPrecisionRecallTrendFiles(min_start_time_ms, max_finish_time_ms, step_ms,
                                 result, &estimator, nopenmp_threads);
    int ctr = 0;
    for(double finish_time_ms=min_start_time_ms+step_ms;
            finish_time_ms<=max_finish_time_ms; finish_time_ms += step_ms){
        cout << "Finish time (ms) " << finish_time_ms << " " << result[ctr++] << endl;
    }
}

void SweepParamsBayesianNet(double min_start_time_ms, double max_finish_time_ms, int nopenmp_threads){
    vector<vector<double> > params;
    for (double p1c = 1.0e-3; p1c <= 5.0e-3; p1c += 0.5e-3){
        for (double p2 = 1.0e-4; p2 <= 5.0e-4; p2 += 0.5e-4){
        //double p2 = 7.5e-4;{
            params.push_back(vector<double> {1.0 - p1c, p2});
        }
    }
    //params = {{1.0 - 3.0e-3, 3.0e-4}};
    vector<PDD> result;
    BayesianNet estimator;
    GetPrecisionRecallParamsFiles(min_start_time_ms, max_finish_time_ms, params,
                                  result, &estimator, nopenmp_threads);
    int ctr = 0;
    for (auto &param: params){
        param[0] = 1.0 - param[0];
        auto p_r = result[ctr++];
        if (p_r.first > 0.82 and p_r.second > 0.2)
            cout << param << " " << p_r << endl;
    }
}

void SweepParamsScore(double min_start_time_ms, double max_finish_time_ms, int nopenmp_threads){
    double min_fail_threshold = 1; //0.001;
    double max_fail_threshold = 16; //0.0025;
    double step = 0.1; //0.00005;
    vector<vector<double> > params;
    for (double fail_threshold=min_fail_threshold;
                fail_threshold < max_fail_threshold; fail_threshold += step){
        params.push_back(vector<double> {fail_threshold});
    }
    vector<PDD> result;
    Score estimator;
    GetPrecisionRecallParamsFiles(min_start_time_ms, max_finish_time_ms, params,
                                  result, &estimator, nopenmp_threads);
    int ctr = 0;
    for (auto &param: params){
        cout << param << " " << result[ctr++] << endl;
    }
}

void SweepParams007(double min_start_time_ms, double max_finish_time_ms, int nopenmp_threads){
    double min_fail_threshold = 0.0001; //0.00125; //1; //0.001;
    double max_fail_threshold = 0.02; //16; //0.0025;
    double step = 0.0001; //0.00005; //0.1; //0.00005;
    vector<vector<double> > params;
    for (double fail_threshold=min_fail_threshold;
                fail_threshold < max_fail_threshold; fail_threshold += step){
        params.push_back(vector<double> {fail_threshold});
    }
    vector<PDD> result;
    DoubleO7 estimator;
    GetPrecisionRecallParamsFiles(min_start_time_ms, max_finish_time_ms, params,
                                  result, &estimator, nopenmp_threads);
    int ctr = 0;
    for (auto &param: params){
        auto p_r = result[ctr++];
        if (p_r.first > 0.1 or p_r.second > 0.05)
            cout << param << " " << p_r << endl;
    }
}

int main(int argc, char *argv[]){
    assert (argc == 5);
    double min_start_time_ms = atof(argv[1]) * 1000.0, max_finish_time_ms = atof(argv[2]) * 1000.0;
    double step_ms = atof(argv[3]) * 1000.0;
    int nopenmp_threads = atoi(argv[4]);
    cout << "Using " << nopenmp_threads << " openmp threads"<<endl;
    //GetPrecisionRecallTrendBayesianNet(min_start_time_ms, max_finish_time_ms,
    //                                   step_ms, nopenmp_threads);
    //GetPrecisionRecallTrend007(min_start_time_ms, max_finish_time_ms,
    //                                   step_ms, nopenmp_threads);
    SweepParamsBayesianNet(min_start_time_ms, max_finish_time_ms, nopenmp_threads);
    return 0;
}
