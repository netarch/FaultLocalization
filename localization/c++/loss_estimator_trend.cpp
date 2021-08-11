#include <iostream>
#include <assert.h>
#include "utils.h"
#include "common_testing_system.h"
#include <chrono>
#include "bayesian_net.h"
#include "bayesian_net_sherlock.h"
#include "doubleO7.h"
#include "net_bouncer.h"
#include "score.h"

using namespace std;

vector<string> GetFilesHwCalibration(){
    //string file_prefix = "/home/vharsh2/flock/ns3/topology/hw_ls_6_2/optical_fault/plog_testbed";
    //string file_prefix = "/home/vharsh2/Flock/ns3/topology/hw_ls_6_2/calibration/wred_logs/plog_nb";
    string file_prefix = "/home/vharsh2/Flock/ns3/topology/hw_ls_6_2/calibration/packet_corruption_logs/t60/plog_nb";
    vector<PII> ignore_files = {};
    vector<string> files;
    for(int f=1; f<=3; f++){
    //for (int f: vector<int>({8})){ // {1-8}
        //for(int s: vector<int>({1,2,3,4,5,6,7,8})){ // {1-2}
        for (int s = 1; s < 32; s++){
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" + to_string(s)); 
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    //files = {file_prefix + "_1_0_1"};
    return files;
}

vector<string> GetFilesHw(){
    //string file_prefix = "/home/vharsh2/flock/ns3/topology/hw_ls_6_2/optical_fault/plog_testbed";
    string file_prefix = "/home/vharsh2/Flock/ns3/topology/hw_ls_6_2/hw_logs/plog_testbed";
    vector<PII> ignore_files = {};
    vector<string> files;
    for(int f=0; f<=2; f++){
    //for (int f: vector<int>({8})){ // {1-8}
        //for(int s: vector<int>({1,2,3,4,5,6,7,8})){ // {1-2}
        for (int s = 0; s < 99; s++){
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" + to_string(s)); 
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    //files = {file_prefix + "_1_0_1"};
    return files;
}

vector<string> GetFilesMixed40G(){
    string file_prefix = "/home/vharsh2/Flock/ns3/topology/ft_k10_os3/logs/40G/plog_nb";
    vector<PII> ignore_files = {PII(8, 3)};
    vector<string> files;
    for(int f=1; f<=8; f++){
    //for (int f: vector<int>({8})){ // {1-8}
        for(int s: vector<int>({1,2,3,4,5,6,7,8})){ //1,2,3,4,5,6,7,8})){ // {1-2}
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_f" + to_string(f) + "_0_s" + to_string(s)); 
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    return files;
}

vector<string> GetFilesMixed(){
    //string file_prefix = "/home/vharsh2/Flock/ns3/topology/hw_ls_6_2/traffic_files/plog";
    string file_prefix = "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/ns3/topology/ft_k10_os3/traffic_files_nb/plog_nb";
    vector<PII> ignore_files = {};
    vector<string> files;
    for(int f=1; f<=8; f++){
    //for (int f: vector<int>({8})){ // {1-8}
        for(int s: vector<int>({5,6,7,8})){ //1,2,3,4,5,6,7,8})){ // {1-2}
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
    //string file_prefix = "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/flow_simulator/logs/irregular_topology/plog";
    string file_prefix = "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/ns3/topology/ft_k10_os3/ommitted/logs/plog";
    int nlinks_ommitted = 100;
    typedef array<int, 3> AI3;
    vector<AI3> ignore_files = {{10, 8, 3}, {30, 7, 3}, {40, 7, 3}, {50, 7, 3}};
    vector<string> files;
    for(int s: vector<int>({1,2,3,4})){ // {1-2}
        for(int f=1; f<=8; f++){
            if(find(ignore_files.begin(), ignore_files.end(),  AI3({nlinks_ommitted, f, s})) == ignore_files.end()){
                files.push_back(file_prefix + "_o" + to_string(nlinks_ommitted) + "_" + to_string(f) + "_0_" + to_string(s)); 
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
    //string file_prefix = "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/topology/ft_k10_os3/softness_logs/random_traffic_logs/plog";
    vector<string> loss_rate_strings;
    string file_prefix;
    vector<pair<string, int> > ignore_files;
    vector<string> files;

    file_prefix = "/home/vharsh2/Flock/ns3/topology/ft_k10_os3/logs/40G/softness/plog_nb_random";
    loss_rate_strings = {"0.014", "0.018"};
    //loss_rate_strings = {"0.01"};
    ignore_files = {{"0.002",39}, {"0.004",18}, {"0.004",19}, {"0.004",8}, {"0.014",21}, {"0.014",24}, {"0.014",28}, {"0.014",29}};
    for (string& loss_rate_string: loss_rate_strings){
        for(int i=1; i<=16; i++){
            if(find(ignore_files.begin(), ignore_files.end(),  pair<string, int>(loss_rate_string, i)) == ignore_files.end()){
                files.push_back(file_prefix + "_" + loss_rate_string + "_" + to_string(i)); 
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    file_prefix = "/home/vharsh2/Flock/ns3/topology/ft_k10_os3/logs/40G/softness/plog_nb_skewed";
    loss_rate_strings = {"0.018"};
    ignore_files = {{"0.014",3}, {"0.004",19}, {"0.004",2}, {"0.004",22}, {"0.004",6}};
    for (string& loss_rate_string: loss_rate_strings){
        for(int i=1; i<=16; i++){
            if(find(ignore_files.begin(), ignore_files.end(),  pair<string, int>(loss_rate_string, i)) == ignore_files.end()){
                files.push_back(file_prefix + "_" + loss_rate_string + "_" + to_string(i)); 
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
    //return GetFilesHwCalibration();
    //return GetFilesHw();
    //return GetFilesRRG();
    //return GetFilesMixed();
    return GetFilesMixed40G();
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

void GetPrecisionRecallTrendSherlock(string topology_filename, double min_start_time_ms, 
                           double max_finish_time_ms, double step_ms, int nopenmp_threads){
    vector<PDD> result;
    Sherlock estimator;
    vector<double> param = {1.0-5.0e-3, 2.0e-4, -25.0};
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
    for (double p1c = 2.0e-3; p1c <=10.0e-3+eps; p1c += 1.0e-3){
        //for (double p2 = 2.0e-4; p2 <=7.0e-4+eps; p2 += 0.5e-4){
        for (double p2 = 0.5e-4; p2 <=5.5e-4+eps;p2 += 0.5e-4){
            if (p2 >= p1c - 0.5e-5) continue;
            //double nprior = 10.0;
            //for(double nprior=7.5; nprior<=22.5+eps; nprior+=2.5)
            //for (double nprior: {10.0, 25.0, 50.0, 100.0, 250.0, 500.0})
            for(double nprior=0.0; nprior<=50.0+eps; nprior+=5.0)
                params.push_back(vector<double> {1.0 - p1c, p2, -nprior});
            //}
        }
    }
    /*
    for (double p1c = 2.0e-4; p1c <=7.0e-4+eps; p1c += 1.0e-4){
        for (double p2 = 2.0e-5; p2 <=7.0e-5+eps; p2 += 1.0e-5){
            if (p2 >= p1c - 0.5e-4) continue;
            for (double nprior: {5.0, 10.0, 25.0, 50.0})
                params.push_back(vector<double> {1.0 - p1c, p2, -nprior});
        }
    }
    */
    //params = {{1.0 - 4.0e-3, 2.0e-4, -250.0}};
    //params = {{1.0 - 0.01, 0.00055, -5}}; //flock (A1)
    //params = {{1.0 - 0.008, 0.00035, -7.5}}; //flock (A1)
    //params = {{1.0 - 0.01, 0.0003, -35}}; //flock (A2)
    //params = {{1.0- 0.005, 0.0005, -7.5}}; //Flock (A1+A2+P)
    params = {{1.0 - 0.003, 0.0002, -20}}; // Flock (A1+A2+P)
    vector<PDD> result;
    BayesianNet estimator;
    GetPrecisionRecallParamsFiles(topology_filename, min_start_time_ms, max_finish_time_ms,
                                  params, result, &estimator, nopenmp_threads);
    int ctr = 0;
    for (auto &param: params){
        param[0] = 1.0 - param[0];
        auto p_r = result[ctr++];
        if (p_r.first > 0.0 and p_r.second > 0.0)
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
    double min_fail_threshold = 0.0001; //0.00125; //1; //0.001;
    double max_fail_threshold = 0.20; //16; //0.0025;
    double step = 0.0001; ///0.1; //0.00005;
    vector<vector<double> > params;
    for (double fail_threshold=min_fail_threshold;
                fail_threshold < max_fail_threshold; fail_threshold += step){
        params.push_back(vector<double> {fail_threshold});
    }
    //params = {{0.0025}}; //random
    //params = {{0.0083}}; //skewed

    vector<PDD> result;
    DoubleO7 estimator;
    GetPrecisionRecallParamsFiles(topology_filename, min_start_time_ms, max_finish_time_ms,
                                  params, result, &estimator, nopenmp_threads);
    int ctr = 0;
    for (auto &param: params){
        auto p_r = result[ctr++];
        if (p_r.first > 0.0 or p_r.second > 0.0)
            cout << param << " " << p_r << endl;
    }
}

void SweepParamsNetBouncer(string topology_filename, double min_start_time_ms, double max_finish_time_ms, int nopenmp_threads){
    vector<vector<double> > params;
    for (double regularize_c = 1.0e-3; regularize_c <= 25.0e-3; regularize_c += 2.5e-3){
        //for (double fail_threshold_c = 5.0e-4; fail_threshold_c <= 10.0e-3; fail_threshold_c += 5e-4){
        for (double fail_threshold_c = 10.0e-3; fail_threshold_c <= 15.0e-3; fail_threshold_c += 10e-4){
            params.push_back(vector<double> {regularize_c, fail_threshold_c});
        }
    }
    params = {{0.016, 0.0113}};
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
    //GetPrecisionRecallTrendSherlock(topology_filename, min_start_time_ms, 
                                    //max_finish_time_ms, step_ms, nopenmp_threads);
    SweepParamsBayesianNet(topology_filename, min_start_time_ms, max_finish_time_ms, nopenmp_threads);
    return 0;
}
