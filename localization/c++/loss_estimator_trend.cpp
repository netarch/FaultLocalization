#include <iostream>
#include <assert.h>
#include "utils.h"
#include "common_testing_system.h"
#include <chrono>
#include "bayesian_net.h"
#include "doubleO7.h"
#include "net_bouncer.h"
#include "score.h"

using namespace std;


vector<string> GetFilesMixed(){
    string file_prefix = "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/ns3/topology/ft_k10_os3/traffic_files_nb/plog_nb";
    vector<PII> ignore_files = {};
    vector<string> files;
    for(int f=1; f<=8; f++){
    //for (int f: vector<int>({8})){ // {1-8}
        for(int s: vector<int>({1,2,3,4,5,6,7,8})){ // {1-2}
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" + to_string(s)); 
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    //files = {file_prefix + "_1_0_1"};
    return files;
}

vector<string> GetFilesRRG(){
    string file_prefix = "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/flow_simulator/logs/irregular_topology/plog";
    vector<PII> ignore_files = {};
    vector<string> files;
    for(int f=1; f<=8; f++){
    //for (int f: vector<int>({8})){ // {1-8}
        for(int s: vector<int>({1,2,3,4})){ // {1-2}
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_s" + to_string(s) + "_f" + to_string(f)); 
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    //files = {file_prefix + "_1_0_1"};
    return files;
}


vector<string> GetFilesFlowSimulator(){
    string file_prefix = "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/localization/flow_simulator/ft_logs/plog";
    vector<pair<string, int> > ignore_files = {};
    vector<string> files;
    vector<string> loss_rate_strings = {"0.001", "0.002", "0.004"};
    for (string& loss_rate_string: loss_rate_strings){
        for(int i=1; i<=1; i++){
            for(int f: {10}){
                files.push_back(file_prefix + "_" + loss_rate_string + "_" + to_string(f) + "_" + to_string(i)); 
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    vector<string> loss_rate_strings1 = {"0.003", "0.004"};
    for (string& loss_rate_string: loss_rate_strings1){
        for(int i=1; i<=1; i++){
            for(int f: {10}){
                files.push_back(file_prefix + "_min" + loss_rate_string + "_mixed_" + to_string(f) + "_" + to_string(i)); 
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    return files;
}

vector<string> GetFilesSoftness(){
    string file_prefix = "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/topology/ft_k10_os3/softness_logs/random_traffic_logs/plog";
    vector<string> files;
    vector<string> loss_rate_strings = {"0.006","0.014","0.010","0.018","0.022"};
    //vector<string> loss_rate_strings = {"0.022"};
    vector<pair<string, int> > ignore_files = {{"0.014",2},{"0.022",10}};
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
    string loss_rate_string = "0.002";
    for(int i=1; i<=16; i++){
        if(find(ignore_files.begin(), ignore_files.end(),  pair<string, int>(loss_rate_string, i)) == ignore_files.end()){
            files.push_back(file_prefix + "_1_" + loss_rate_string + "_" + to_string(i)); 
            cout << "adding file for analaysis " <<files.back() << endl;
        }
    }
    return files;
}

vector<string> GetFiles(){
    return GetFilesRRG();
    //return GetFilesMixed();
    //return GetFiles007Verification();
    //return GetFilesFlowSimulator();
    //return GetFilesSoftness();
}

void GetPrecisionRecallTrendScore(string topology_filename, double min_start_time_ms,
                     double max_finish_time_ms, double step_ms, int nopenmp_threads){
    vector<PDD> result;
    Score estimator;
    vector<double> param = {1.0};
    estimator.SetParams(param);
    GetPrecisionRecallTrendFiles(topology_filename, min_start_time_ms, max_finish_time_ms, 
                                 step_ms, result, &estimator, nopenmp_threads);
    int ctr = 0;
    for(double finish_time_ms=min_start_time_ms+step_ms;
            finish_time_ms<=max_finish_time_ms; finish_time_ms += step_ms){
        cout << "Finish time (ms) " << finish_time_ms << " " << result[ctr++] << endl;
    }
}

void GetPrecisionRecallTrend007(string topology_filename, double min_start_time_ms,
                    double max_finish_time_ms, double step_ms, int nopenmp_threads){
    vector<PDD> result;
    DoubleO7 estimator;
    vector<double> param = {0.003};
    estimator.SetParams(param);
    GetPrecisionRecallTrendFiles(topology_filename, min_start_time_ms, max_finish_time_ms, 
                                 step_ms, result, &estimator, nopenmp_threads);
    int ctr = 0;
    for(double finish_time_ms=min_start_time_ms+step_ms;
            finish_time_ms<=max_finish_time_ms; finish_time_ms += step_ms){
        cout << "Finish time (ms) " << finish_time_ms << " " << result[ctr++] << endl;
    }
}

void GetPrecisionRecallTrendBayesianNet(string topology_filename, double min_start_time_ms, 
                           double max_finish_time_ms, double step_ms, int nopenmp_threads){
    vector<PDD> result;
    BayesianNet estimator;
    vector<double> param = {1.0-5.0e-3, 2.0e-4, -25.0};
    //vector<double> param = {1.0-2.0e-3, 0.8e-4};
    /*
    if (estimator.USE_CONDITIONAL){
        estimator.PRIOR = -15.0;
        param = {1.0-5.0e-3, 4.0e-4};
    }
    */
    estimator.SetParams(param);
    GetPrecisionRecallTrendFiles(topology_filename, min_start_time_ms, max_finish_time_ms, 
                                 step_ms, result, &estimator, nopenmp_threads);
    int ctr = 0;
    for(double finish_time_ms=min_start_time_ms+step_ms;
            finish_time_ms<=max_finish_time_ms; finish_time_ms += step_ms){
        cout << "Finish time (ms) " << finish_time_ms << " " << result[ctr++] << endl;
    }
}

void SweepParamsBayesianNet(string topology_filename, double min_start_time_ms, double max_finish_time_ms, int nopenmp_threads){
    vector<vector<double> > params;
    double eps = 1.0e-10;
    for (double p1c = 1.0e-3; p1c <=5e-3+eps; p1c += 1.0e-3){
        for (double p2 = 1.0e-4; p2 <= 4.0e-4+eps; p2 += 0.5e-4){
            if (p2 >= p1c - 0.25e-4) continue;
            double nprior = 25.0;
            //for(double nprior=5.0; nprior<=35.0+eps; nprior+=10.0){
                params.push_back(vector<double> {1.0 - p1c, p2, -nprior});
            //}
        }
    }
    //params = {{1.0 - 5.0e-3, 2.0e-4, -25.0}};
    vector<PDD> result;
    BayesianNet estimator;
    GetPrecisionRecallParamsFiles(topology_filename, min_start_time_ms, max_finish_time_ms,
                                  params, result, &estimator, nopenmp_threads);
    int ctr = 0;
    for (auto &param: params){
        param[0] = 1.0 - param[0];
        auto p_r = result[ctr++];
        if (p_r.first > 0.3 and p_r.second > 0.3)
            cout << param << " " << p_r << endl;
    }
}

void SweepParamsScore(string topology_filename, double min_start_time_ms, double max_finish_time_ms, int nopenmp_threads){
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
    GetPrecisionRecallParamsFiles(topology_filename, min_start_time_ms, max_finish_time_ms,
                                  params, result, &estimator, nopenmp_threads);
    int ctr = 0;
    for (auto &param: params){
        cout << param << " " << result[ctr++] << endl;
    }
}

void SweepParams007(string topology_filename, double min_start_time_ms, double max_finish_time_ms, int nopenmp_threads){
    double min_fail_threshold = 0.02; //0.00125; //1; //0.001;
    double max_fail_threshold = 0.10; //16; //0.0025;
    double step = 0.001; ///0.1; //0.00005;
    vector<vector<double> > params;
    for (double fail_threshold=min_fail_threshold;
                fail_threshold < max_fail_threshold; fail_threshold += step){
        params.push_back(vector<double> {fail_threshold});
    }
    vector<PDD> result;
    DoubleO7 estimator;
    GetPrecisionRecallParamsFiles(topology_filename, min_start_time_ms, max_finish_time_ms,
                                  params, result, &estimator, nopenmp_threads);
    int ctr = 0;
    for (auto &param: params){
        auto p_r = result[ctr++];
        if (p_r.first > 0.1 or p_r.second > 0.1)
            cout << param << " " << p_r << endl;
    }
}

void SweepParamsNetBouncer(string topology_filename, double min_start_time_ms, double max_finish_time_ms, int nopenmp_threads){
    vector<vector<double> > params;
    for (double regularize_c = 1.0e-3; regularize_c <= 25.0e-3; regularize_c += 2.5e-3){
        for (double fail_threshold_c = 5.0e-4; fail_threshold_c <= 5.0e-3; fail_threshold_c += 2.5e-4){
            params.push_back(vector<double> {regularize_c, fail_threshold_c});
        }
    }
    vector<PDD> result;
    NetBouncer estimator;
    GetPrecisionRecallParamsFiles(topology_filename, min_start_time_ms, max_finish_time_ms,
                                  params, result, &estimator, nopenmp_threads);
    int ctr = 0;
    for (auto &param: params){
        auto p_r = result[ctr++];
        cout << param << " " << p_r << endl;
    }
}

int main(int argc, char *argv[]){
    assert (argc == 6);
    string topology_filename (argv[1]);
    cout << "Reading topology from file " << topology_filename << endl;
    double min_start_time_ms = atof(argv[2]) * 1000.0, max_finish_time_ms = atof(argv[3]) * 1000.0;
    double step_ms = atof(argv[4]) * 1000.0;
    int nopenmp_threads = atoi(argv[5]);
    cout << "Using " << nopenmp_threads << " openmp threads"<<endl;
    cout << "sizeof(Flow) " << sizeof(Flow) << " bytes" << endl;
    GetPrecisionRecallTrendBayesianNet(topology_filename, min_start_time_ms, 
                                       max_finish_time_ms, step_ms, nopenmp_threads);
    //SweepParamsBayesianNet(topology_filename, min_start_time_ms, max_finish_time_ms, nopenmp_threads);
    return 0;
}
