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


bool USE_DIFFERENT_TOPOLOGIES = false;

bool CROSS_VALIDATION = true;
bool TRAINING_SET = false;

vector<string> GetFilesLinkFlap(){
    string file_prefix = "/home/vharsh2/Flock/hw_traces/link_flap_traces/plog_skewed";
    vector<PII> ignore_files = {};
    vector<string> files;
    for(int f=0; f<=1; f++){
        //for (int f: vector<int>({8})){ // {1-8}
        //for(int s: vector<int>({1,2,3,4,5,6,7,8})){ // {1-2}
        for (int s = 0; s < 25; s++){
            if (CROSS_VALIDATION and (s+(int)TRAINING_SET)%2==0) continue;
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" + to_string(s));
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    ignore_files = {};
    file_prefix = "/home/vharsh2/Flock/hw_traces/link_flap_traces/plog_random";
    for(int f=0; f<=1; f++){
        for (int s = 0; s < 25; s++){
            if (CROSS_VALIDATION and (s+(int)TRAINING_SET)%2==0) continue;
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" + to_string(s));
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    return files;
}





vector<string> GetFilesPacketCorruption(){
    string base_dir = "/home/vharsh2//Flock/hw_traces/emulated";
    string file_prefix = base_dir + "/drop_prob_0.5/tor_uplink/test_random";
    vector<PII> ignore_files = {};
    vector<string> files;  
    for(int f=1; f<=1; f++){
        //for (int f: vector<int>({8})){ // {1-8}
        //for(int s: vector<int>({1,2,3,4,5,6,7,8})){ // {1-2}
        for (int s = 0; s < 30; s++){
            if (CROSS_VALIDATION and (s+(int)TRAINING_SET)%2==0) continue;
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_" + to_string(f) + "_" + to_string(s));
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    ignore_files = {};
    file_prefix = base_dir + "/drop_prob_0.5/tor_uplink/test_skewed";
    for(int f=1; f<=1; f++){
        for (int s = 0; s < 30; s++){
            if (CROSS_VALIDATION and (s+(int)TRAINING_SET)%2==0) continue;
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_" + to_string(f) + "_" + to_string(s));
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    ignore_files = {};
    file_prefix = base_dir + "/drop_prob_0.5/host_uplink/test_random";
    for(int f=1; f<=1; f++){
        for (int s = 0; s < 30; s++){
            if (CROSS_VALIDATION and (s+(int)TRAINING_SET)%2==0) continue;
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_" + to_string(f) + "_" + to_string(s));
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    ignore_files = {};
    file_prefix = base_dir + "/drop_prob_0.5/host_uplink/test_skewed";
    for(int f=1; f<=1; f++){
        for (int s = 0; s < 30; s++){
            if (CROSS_VALIDATION and (s+(int)TRAINING_SET)%2==0) continue;
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_" + to_string(f) + "_" + to_string(s));
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    ignore_files = {};
    file_prefix = base_dir + "/no_failure/test_random";
    for(int f=0; f<=0; f++){
        int smin = (CROSS_VALIDATION? 0: 15); //if not cross validation, then hybrid calibration
        for (int s = smin; s < 30; s++){
            if (CROSS_VALIDATION and (s+(int)TRAINING_SET)%2==0) continue;
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_" + to_string(f) + "_" + to_string(s));
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    ignore_files = {};
    file_prefix = base_dir + "/no_failure/test_skewed";
    for(int f=0; f<=0; f++){
        int smin = (CROSS_VALIDATION? 0: 15); //if not cross validation, then hybrid calibration
        for (int s = smin; s < 30; s++){
            if (CROSS_VALIDATION and (s+(int)TRAINING_SET)%2==0) continue;
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_" + to_string(f) + "_" + to_string(s));
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    return files;
}

vector<string> GetFilesWred(){
    //string file_prefix = "/home/vharsh2/Flock/ns3/topology/hw_ls_6_2/optical_fault/plog_testbed";
    //string file_prefix = "collector/logs/ls_6_2_minus_h39/t30s/plog_testbed";
    //string file_prefix = "collector/logs/ls_6_2/hw_logs/plog_testbed";
    vector<PII> ignore_files = {{1,6}};
    vector<string> files;
    string file_prefix = "/home/vharsh2/Flock/hw_traces/wred_traces/plog_skewed";
    for(int f=0; f<=2; f++){
        //for (int f: vector<int>({8})){ // {1-8}
        //for(int s: vector<int>({1,2,3,4,5,6,7,8})){ // {1-2}
        int smin = (f==0 and !CROSS_VALIDATION? 14: 0);
        for (int s = smin; s < 30; s++){
            if (CROSS_VALIDATION and (s+(int)TRAINING_SET)%2==0) continue;
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" + to_string(s));
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    ignore_files = {};
    file_prefix = "/home/vharsh2/Flock/hw_traces/wred_traces/plog_random";
    for(int f=0; f<=2; f++){
        int smin = (f==0 and !CROSS_VALIDATION? 14: 0);
        for (int s = smin; s < 30; s++){
            if (CROSS_VALIDATION and (s+(int)TRAINING_SET)%2==0) continue;
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" + to_string(s));
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    return files;
}

pair<vector<string>, vector<string> > GetFilesHwCalibrationWred(){
    string file_prefix = "/home/vharsh2/Flock/ns3/topology/hw_ls_6_2/calibration/wred_logs/plog_nb";
    string topology = "/home/vharsh2/Flock/ns3/topology/hw_ls_6_2/calibration/ns3ls_x6_y2_i1.edgelist";
    vector<PII> ignore_files = {};
    vector<string> files, topologies;
    for(int f=1; f<=3; f++){
    //for (int f: vector<int>({8})){ // {1-8}
        //for(int s: vector<int>({1,2,3,4,5,6,7,8})){ // {1-2}
        for (int s = 1; s < 25; s++){
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" + to_string(s)); 
                topologies.push_back(topology);
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    topology = "/home/vharsh2/Flock/hw_traces/wred_traces/ls_6_2.edgelist";
    ignore_files = {{1,6}};
    file_prefix = "/home/vharsh2/Flock/hw_traces/wred_traces/plog_skewed";
    for(int f=0; f<=0; f++){
        for (int s = 0; s < 14; s++){
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" + to_string(s));
                topologies.push_back(topology);
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    ignore_files = {};
    file_prefix = "/home/vharsh2/Flock/hw_traces/wred_traces/plog_random";
    for(int f=0; f<=0; f++){
        for (int s = 0; s < 14; s++){
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" + to_string(s));
                topologies.push_back(topology);
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    return pair<vector<string>, vector<string> >(files, topologies);
}

pair<vector<string>, vector<string> > GetFilesHwCalibrationPacor(){
    string file_prefix = "/home/vharsh2/Flock/ns3/topology/hw_ls_6_2/calibration/packet_corruption_logs/t60/plog_nb";
    string topology = "/home/vharsh2/Flock/ns3/topology/hw_ls_6_2/calibration/ns3ls_x6_y2_i1.edgelist";
    vector<PII> ignore_files = {};
    vector<string> files, topologies;
    for(int f=1; f<=3; f++){
    //for (int f: vector<int>({8})){ // {1-8}
        //for(int s: vector<int>({1,2,3,4,5,6,7,8})){ // {1-2}
        for (int s = 1; s < 25; s++){
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" + to_string(s)); 
                topologies.push_back(topology);
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    ignore_files = {};
    string base_dir = "/home/vharsh2//Flock/hw_traces/emulated";
    topology = base_dir + "/ls_6_2.edgelist";
    file_prefix = base_dir + "/no_failure/test_random";
    for(int f=0; f<=0; f++){
        for (int s = 0; s < 15; s++){
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_" + to_string(f) + "_" + to_string(s));
                topologies.push_back(topology);
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    ignore_files = {};
    file_prefix = base_dir + "/no_failure/test_skewed";
    for(int f=0; f<=0; f++){
        for (int s = 0; s < 15; s++){
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_" + to_string(f) + "_" + to_string(s));
                topologies.push_back(topology);
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    return pair<vector<string>, vector<string> >(files, topologies);
}


vector<string> GetFilesHwCalibration(){
    //string file_prefix = "/home/vharsh2/flock/ns3/topology/hw_ls_6_2/optical_fault/plog_testbed";
    //string file_prefix = "/home/vharsh2/Flock/ns3/topology/hw_ls_6_2/calibration/wred_logs/plog_nb";
    string file_prefix = "/home/vharsh2/Flock/ns3/topology/hw_ls_6_2/calibration/packet_corruption_logs/t60/plog_nb";
    vector<PII> ignore_files = {};
    vector<string> files;
    for(int f=1; f<=3; f++){
    //for (int f: vector<int>({8})){ // {1-8}
        //for(int s: vector<int>({1,2,3,4,5,6,7,8})){ // {1-2}
        for (int s = 1; s < 25; s++){
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
    string file_prefix = "/home/vharsh2/Flock/hw_traces/wred/plog";
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

vector<string> GetFilesDevice40G(){
    string file_dir = "/home/vharsh2/Flock/ns3/topology/ft_k10_os3/logs/40G/device";
    vector<PII> ignore_files = {};
    vector<string> files;
    for (string frac: vector<string>({"0.25", "0.5", "0.75", "1"})){
        string file_prefix = file_dir + "/frac_" + frac + "/plog_nb";
        for(int f=1; f<=2; f++){
        //for (int f: vector<int>({8})){ // {1-8}
            for(int s=1; s<=8; s++){
                if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                    files.push_back(file_prefix + "_f" + to_string(f) + "_0_s" + to_string(s)); 
                    cout << "adding file for analaysis " <<files.back() << endl;
                }
            }
        }
    }
    return files;
}

vector<string> GetFilesMisconfiguredAcl(){
    string file_prefix = "/home/vharsh2/Flock/flow_simulator/logs/macl/plog_macl";
    vector<PII> ignore_files = {};
    vector<string> files;
    for(int f=1; f<=2; f++){
    //for (int f: vector<int>({8})){ // {1-8}
        for(int s=1; s<=16; s++){ //1,2,3,4,5,6,7,8})){ // {1-2}
            if(find(ignore_files.begin(), ignore_files.end(),  PII(f, s)) == ignore_files.end()){
                files.push_back(file_prefix + "_f" + to_string(f) + "_0_s" + to_string(s)); 
                cout << "adding file for analaysis " <<files.back() << endl;
            }
        }
    }
    return files;
}

vector<string> GetFilesMixed40G(){
    assert (CROSS_VALIDATION);
    string file_prefix = "/home/vharsh2/Flock/ns3/topology/ft_k10_os3/logs/40G/plog_nb";
    vector<PII> ignore_files = {PII(8, 3)};
    vector<string> files;
    for(int f=1; f<=8; f++){
    //for (int f: vector<int>({8})){ // {1-8}
        for(int s: vector<int>({1,2,3,4,5,6,7,8})){ //1,2,3,4,5,6,7,8})){ // {1-2}
            if ((s + (int)TRAINING_SET) % 2 == 0) continue;
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

pair<vector<string>, vector<string> > GetFilesRRG(){
    //string file_prefix = "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/flow_simulator/logs/irregular_topology/plog";
    string file_prefix = "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/ns3/topology/ft_k10_os3/ommitted/logs/new/plog";
    int nlinks_ommitted = 100;
    string topo_prefix = "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/ns3/topology/ft_k10_os3/ommitted/topologies/ns3ft_o"
                         + to_string(nlinks_ommitted) + "_deg10_sw125_svr250_os3";
    typedef array<int, 3> AI3;
    vector<AI3> ignore_files = {};
    vector<string> files, topologies;
    //for(int s: vector<int>({1,7})){ // {1-2}
    for(int s: vector<int>({2,3,4,5,6})){ // {1-2}
        for(int f=1; f<=8; f++){
            if(find(ignore_files.begin(), ignore_files.end(),  AI3({nlinks_ommitted, f, s})) == ignore_files.end()){
                files.push_back(file_prefix + "_o" + to_string(nlinks_ommitted) + "_" + to_string(f) + "_0_" + to_string(s));
                topologies.push_back(topo_prefix + "_fail" + to_string(f) + "_seed" + to_string(s) + ".edgelist");
                cout << "adding file for analaysis " <<files.back() << " " << topologies.back() << endl;
            }
        }
    }
    return {files, topologies};
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

string SOFTNESS_FILE_PREFIX;
vector<string> GetFilesSoftness(){
    vector<string> loss_rate_strings;
    string file_prefix;
    vector<pair<string, int> > random_ignore_files, skewed_ignore_files, ignore_files;
    vector<string> files;

    random_ignore_files = {{"0.002",39}, {"0.004",18}, {"0.004",19}, {"0.004",8}, {"0.014",21}, {"0.014",24}, {"0.014",28}, {"0.014",29}};
    skewed_ignore_files = {{"0.014",3}, {"0.004",19}, {"0.004",2}, {"0.004",22}, {"0.004",6}};
    if (SOFTNESS_FILE_PREFIX.find("random") != string::npos) ignore_files = random_ignore_files;
    else if (SOFTNESS_FILE_PREFIX.find("skewed") != string::npos) ignore_files = skewed_ignore_files;
    else assert(false);

    for(int i=1; i<=40; i++){
        bool ignore = false;
        for (auto [slr, ig]: ignore_files){
            if (i==ig and SOFTNESS_FILE_PREFIX.find("_" + slr + "_") != string::npos) ignore = true;
        }
        if (!ignore){
            files.push_back(SOFTNESS_FILE_PREFIX + "_" + to_string(i)); 
            cout << "adding file for analaysis " <<files.back() << endl;
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

pair<vector<string>, vector<string> > GetFilesTopologies(){
    //return GetFilesRRG();
    //return GetFilesHwCalibrationWred();
    return GetFilesHwCalibrationPacor();
}

vector<string> GetFilesHelper(){
    //return GetFilesHwCalibration();
    //return GetFilesHw();
    //return GetFilesRRG();
    //return GetFilesMixed();
    //return GetFilesMixed40G();
    //return GetFilesDevice40G();
    //return GetFilesMisconfiguredAcl();
    //return GetFiles007Verification();
    //return GetFilesFlowSimulator();
    //return GetFilesSoftness();
    //return GetFilesWred();
    return GetFilesPacketCorruption();
}

vector<string> (*GetFiles)() = GetFilesHelper;

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
    if (INPUT_FLOW_TYPE==PROBLEMATIC_FLOWS){
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

typedef vector<double> ParamType;
vector<tuple<ParamType, PDD> > SweepParamsBayesianNet(string topology_filename,
                                            double min_start_time_ms, double max_finish_time_ms,
                                            int nopenmp_threads){
    vector<ParamType> params;
    double eps = 1.0e-10;
    /*
    for (double p1c = 0.5e-3; p1c <=4.0e-3+eps; p1c += 1.0e-3){
        //for (double p2 = 2.0e-4; p2 <=7.0e-4+eps; p2 += 0.5e-4){
        for (double p2 = 0.25e-4; p2 <=1.5e-4+eps;p2 += 0.25e-4){
            if (p2 >= p1c - 0.5e-5) continue;
            //double nprior = 10.0;
            //for(double nprior=7.5; nprior<=22.5+eps; nprior+=2.5)
            //for (double nprior: {10.0, 25.0, 50.0, 100.0, 250.0, 500.0})
            for(double nprior=0.0; nprior<=5.0+eps; nprior+=1.0)
            //for(double nprior=0.0; nprior<=25.0+eps; nprior+=5.0)
                params.push_back(vector<double> {1.0 - p1c, p2, -nprior});
            //}
        }
    }
    */
    /*
    for (double p1c = 2.0e-4; p1c <=7.0e-4+eps; p1c += 1.0e-4){
        for (double p2 = 2.0e-5; p2 <=7.0e-5+eps; p2 += 1.0e-5){
            if (p2 >= p1c - 0.5e-4) continue;
            //for (double nprior: {5.0, 10.0, 25.0, 50.0})
            for (double nprior: {10.0, 25.0})
                params.push_back(vector<double> {1.0 - p1c, p2, -nprior});
        }
    }
    */
    for (double p1c = 2.0e-3; p1c <=15.0e-3+eps; p1c += 1.0e-3){
        for (double p2 = 1.0e-4; p2 <=8.0e-4+eps; p2 += 1.0e-4){
            if (p2 >= p1c - 0.5e-4) continue;
            //for (double nprior: {5.0, 10.0, 25.0, 50.0})
            for (double nprior: {10, 15, 20, 25, 30, 35, 40, 45, 50, 100, 150, 200})
                params.push_back(vector<double> {1.0 - p1c, p2, -nprior});
        }
    }
    //params = {{1.0 - 4.0e-3, 2.0e-4, -250.0}};
    //params = {{1.0 - 0.005, 0.00015, -10}}; //Flock (A1)
    //params = {{1.0 - 0.002, 0.00045, -10.0}}; //Flock (INT) (A1+A2+P)
    //params = {{1.0 - 0.007, 0.00045, -30.0}}; //Flock (A2)
    //params = {{1.0- 0.005, 0.0005, -7.5}}; //Flock (A1+A2+P)
    //params = {{1.0-0.009, 0.0004, -40}}; //flock_omitted
    //params = {{1.0-0.003, 0.0001, -250}};
    vector<PDD> result;
    BayesianNet estimator;
    GetPrecisionRecallParamsFiles(topology_filename, min_start_time_ms, max_finish_time_ms,
                                  params, result, &estimator, nopenmp_threads);
    vector<tuple<ParamType, PDD> > ret;
    int ctr = 0;
    for (auto &param: params){
        param[0] = 1.0 - param[0];
        auto p_r = result[ctr++];
        //if (p_r.first > 0.0 and p_r.second > 0.0)
        cout << param << " " << p_r << endl;
        ret.push_back(make_tuple(param, p_r));
    }
    return ret;
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

vector<tuple<ParamType, PDD> > SweepParams007(string topology_filename, double min_start_time_ms, double max_finish_time_ms, int nopenmp_threads){
    double min_fail_threshold = 0.0001; //0.00125; //1; //0.001;
    double max_fail_threshold = 0.30; //16; //0.0025;
    double step = 0.0001; ///0.1; //0.00005;
    vector<ParamType> params;
    for (double fail_threshold=min_fail_threshold;
                fail_threshold < max_fail_threshold; fail_threshold += step){
        params.push_back(vector<double> {fail_threshold});
    }
    //params = {{0.0019}}; //random
    //params = {{0.0083}}; //skewed
    //params = {{0.0594}};

    vector<PDD> result;
    DoubleO7 estimator;
    GetPrecisionRecallParamsFiles(topology_filename, min_start_time_ms, max_finish_time_ms,
                                  params, result, &estimator, nopenmp_threads);
    int ctr = 0;
    vector<tuple<ParamType, PDD> > res;
    for (auto &param: params){
        auto p_r = result[ctr++];
        if (p_r.first > 0.0 or p_r.second > 0.0){
            cout << param << " " << p_r << endl;
            res.push_back(make_tuple(param, p_r));
        }
    }
    return res;
}

vector<tuple<ParamType, PDD> > SweepParamsNetBouncer(string topology_filename, double min_start_time_ms, double max_finish_time_ms, int nopenmp_threads){
    vector<ParamType> params;
    for (double bad_device_threshold = 0.075; bad_device_threshold <= 0.21; bad_device_threshold += 0.05){
    //for (double bad_device_threshold = 0.01; bad_device_threshold <= 0.31; bad_device_threshold += 0.05){
        //for (double regularize_c = 0.025; regularize_c <= 0.51; regularize_c += 0.025){
        for (double regularize_c = 0.001; regularize_c <= 0.025; regularize_c += 0.0025){
        //for (double regularize_c = 0.06; regularize_c <= 4.0; regularize_c += 0.15){
            for (double fail_threshold_c = 5e-4; fail_threshold_c <= 100.0e-4; fail_threshold_c += 5e-4){
            //for (double fail_threshold_c = 1.0e-3; fail_threshold_c <= 15.0e-3; fail_threshold_c += 1.0e-3){
                params.push_back(vector<double> {regularize_c, fail_threshold_c, bad_device_threshold});
            }
        }
    }
    //params = {{0.006, 0.0015, 0.075}}; //INT
    //params = {{0.56, 0.00025, 0.31}};
    //params = {{0.21, 0.001, 0.175}}; //multiplier
    //params = {{0.06, 0.00225, 0.06}};
    //params = {{0.21, 0.001, 0.075}};
    vector<PDD> result;
    NetBouncer estimator;
    GetPrecisionRecallParamsFiles(topology_filename, min_start_time_ms, max_finish_time_ms,
                                  params, result, &estimator, nopenmp_threads);
    int ctr = 0;
    vector<tuple<ParamType, PDD> > res;
    for (auto &param: params){
        auto p_r = result[ctr++];
        cout << param << " " << p_r << endl;
        res.push_back(make_tuple(param, p_r));
    }
    return res;
}

void WriteResultsToFile(string outfile, vector<tuple<ParamType, PDD> > res){
    ofstream of;
    of.open(outfile);
    for(auto &[param, p_r]: res){
        of << param << " " << p_r << endl;
    }
    of.close();
    cout << "Written results to " << outfile << endl;
}


void SweepParamsAllSchemes(string topology_filename, string base_dir, double min_start_time_ms,
                           double max_finish_time_ms, int nopenmp_threads){
    {
        PATH_KNOWN = true; INPUT_FLOW_TYPE = ALL_FLOWS;
        vector<tuple<ParamType, PDD> > res = SweepParamsBayesianNet(topology_filename,
                                             min_start_time_ms, max_finish_time_ms, nopenmp_threads);
        string outfile = base_dir + "bnet_int";
        WriteResultsToFile(outfile, res);
    }
    {
        PATH_KNOWN = false; INPUT_FLOW_TYPE = ALL_FLOWS; TRACEROUTE_BAD_FLOWS = true;
        vector<tuple<ParamType, PDD> > res = SweepParamsBayesianNet(topology_filename,
                                             min_start_time_ms, max_finish_time_ms, nopenmp_threads);
        string outfile = base_dir + "bnet_a1_a2_p";
        WriteResultsToFile(outfile, res);
    }
    {
        PATH_KNOWN = false; INPUT_FLOW_TYPE = ALL_FLOWS; TRACEROUTE_BAD_FLOWS = false;
        vector<tuple<ParamType, PDD> > res = SweepParamsBayesianNet(topology_filename,
                                             min_start_time_ms, max_finish_time_ms, nopenmp_threads);
        string outfile = base_dir + "bnet_a1_p";
        WriteResultsToFile(outfile, res);
    }
    {
        PATH_KNOWN = false; INPUT_FLOW_TYPE = ACTIVE_FLOWS;
        vector<tuple<ParamType, PDD> > res = SweepParamsBayesianNet(topology_filename,
                                             min_start_time_ms, max_finish_time_ms, nopenmp_threads);
        string outfile = base_dir + "bnet_a1";
        WriteResultsToFile(outfile, res);
    }
    {
        PATH_KNOWN = false; INPUT_FLOW_TYPE = PROBLEMATIC_FLOWS; TRACEROUTE_BAD_FLOWS = true;
        vector<tuple<ParamType, PDD> > res = SweepParamsBayesianNet(topology_filename,
                                             min_start_time_ms, max_finish_time_ms, nopenmp_threads);
        string outfile = base_dir + "bnet_a2";
        WriteResultsToFile(outfile, res);
    }
    {
        PATH_KNOWN = false; INPUT_FLOW_TYPE = PROBLEMATIC_FLOWS; TRACEROUTE_BAD_FLOWS = true;
        vector<tuple<ParamType, PDD> > res = SweepParams007(topology_filename,
                                             min_start_time_ms, max_finish_time_ms, nopenmp_threads);
        string outfile = base_dir + "007_a2";
        WriteResultsToFile(outfile, res);
    }
    {
        PATH_KNOWN = true; INPUT_FLOW_TYPE = ALL_FLOWS;
        vector<tuple<ParamType, PDD> > res = SweepParamsNetBouncer(topology_filename,
                                             min_start_time_ms, max_finish_time_ms, nopenmp_threads);
        string outfile = base_dir + "nb_int";
        WriteResultsToFile(outfile, res);
    }
    {
        PATH_KNOWN = false; INPUT_FLOW_TYPE = ACTIVE_FLOWS;
        vector<tuple<ParamType, PDD> > res = SweepParamsNetBouncer(topology_filename,
                                             min_start_time_ms, max_finish_time_ms, nopenmp_threads);
        string outfile = base_dir + "nb_a1";
        WriteResultsToFile(outfile, res);
    }
}

void SetTestbedScenario(string scenario){
    if (scenario == "link_flap") GetFiles = GetFilesLinkFlap;
    else if (scenario == "wred") GetFiles = GetFilesWred;
    else if (scenario == "pacor") GetFiles = GetFilesPacketCorruption;
    else assert(false);
}

void TestbedCrossValidataion(string topology_filename, double min_start_time_ms,
                        double max_finish_time_ms, int nopenmp_threads){
    USE_DIFFERENT_TOPOLOGIES = false;
    CROSS_VALIDATION = true;
    TRAINING_SET = true;

    string scenario = "link_flap";
    SetTestbedScenario(scenario);

    string base_dir = "/home/vharsh2/Flock/localization/c++/results/hw/" + scenario + "/train_";
    SweepParamsAllSchemes(topology_filename, base_dir, min_start_time_ms, max_finish_time_ms, nopenmp_threads);

    TRAINING_SET = false;
    base_dir = "/home/vharsh2/Flock/localization/c++/results/hw/" + scenario + "/test_";
    SweepParamsAllSchemes(topology_filename, base_dir, min_start_time_ms, max_finish_time_ms, nopenmp_threads);
}


void TestbedHybridCalibration(string topology_filename, double min_start_time_ms,
                        double max_finish_time_ms, int nopenmp_threads){
    CROSS_VALIDATION = false;
    USE_DIFFERENT_TOPOLOGIES = true;

    string scenario = "pacor";
    SetTestbedScenario(scenario);

    string base_dir = "/home/vharsh2/Flock/localization/c++/results/hw/" + scenario + "/cal_";
    SweepParamsAllSchemes(topology_filename, base_dir, min_start_time_ms, max_finish_time_ms, nopenmp_threads);

    USE_DIFFERENT_TOPOLOGIES = false;
    base_dir = "/home/vharsh2/Flock/localization/c++/results/hw/" + scenario + "/cal_hw_";
    SweepParamsAllSchemes(topology_filename, base_dir, min_start_time_ms, max_finish_time_ms, nopenmp_threads);
}


void SimulatorMixedTracesExperiments(string topology_filename, double min_start_time_ms,
                                     double max_finish_time_ms, int nopenmp_threads){
    USE_DIFFERENT_TOPOLOGIES = false;
    CROSS_VALIDATION = true;
    GetFiles = GetFilesMixed40G;

    TRAINING_SET = true;
    string base_dir = "/home/vharsh2/Flock/localization/c++/results/nsdi2022/mixed/t1_train_";
    SweepParamsAllSchemes(topology_filename, base_dir, min_start_time_ms, max_finish_time_ms, nopenmp_threads);

    TRAINING_SET = false;
    base_dir = "/home/vharsh2/Flock/localization/c++/results/nsdi2022/mixed/t1_test_";
    SweepParamsAllSchemes(topology_filename, base_dir, min_start_time_ms, max_finish_time_ms, nopenmp_threads);
}

void SoftnessExperiments(string topology_filename, double min_start_time_ms,
                         double max_finish_time_ms, int nopenmp_threads){
    CROSS_VALIDATION = false;
    vector<string> loss_rate_strings = {"0.002", "0.004", "0.006", "0.01", "0.014", "0.018"};

    string file_prefix, base_dir;
    vector<pair<string, int> > ignore_files;

    file_prefix = "/home/vharsh2/Flock/ns3/topology/ft_k10_os3/logs/40G/softness/plog_nb_random";
    for (string& loss_rate_string: loss_rate_strings){
        SOFTNESS_FILE_PREFIX = file_prefix + "_" + loss_rate_string;
        base_dir = "/home/vharsh2/Flock/localization/c++/results/softness/random_" + loss_rate_string + "_";
        SweepParamsAllSchemes(topology_filename, base_dir, min_start_time_ms, max_finish_time_ms, nopenmp_threads);
    }

    file_prefix = "/home/vharsh2/Flock/ns3/topology/ft_k10_os3/logs/40G/softness/plog_nb_skewed";
    for (string& loss_rate_string: loss_rate_strings){
        SOFTNESS_FILE_PREFIX = file_prefix + "_" + loss_rate_string;
        base_dir = "/home/vharsh2/Flock/localization/c++/results/softness/skewed_" + loss_rate_string + "_";
        SweepParamsAllSchemes(topology_filename, base_dir, min_start_time_ms, max_finish_time_ms, nopenmp_threads);
    }
}

int main(int argc, char *argv[]){
    VERBOSE = false;
    assert (argc == 6);
    string topology_filename (argv[1]);
    cout << "Reading topology from file " << topology_filename << endl;
    double min_start_time_ms = atof(argv[2]) * 1000.0, max_finish_time_ms = atof(argv[3]) * 1000.0;
    double step_ms = atof(argv[4]) * 1000.0;
    int nopenmp_threads = atoi(argv[5]);
    cout << "Using " << nopenmp_threads << " openmp threads"<<endl;
    cout << "sizeof(Flow) " << sizeof(Flow) << " bytes" << endl;
    cout << "Analysis from time(ms) " << min_start_time_ms  << " --> " << max_finish_time_ms << endl;
    //GetPrecisionRecallTrendSherlock(topology_filename, min_start_time_ms, 
                                    //max_finish_time_ms, step_ms, nopenmp_threads);
    //SweepParamsNetBouncer(topology_filename, min_start_time_ms, max_finish_time_ms, nopenmp_threads);
    //TestbedCrossValidataion(topology_filename, min_start_time_ms, max_finish_time_ms, nopenmp_threads);
    SimulatorMixedTracesExperiments(topology_filename, min_start_time_ms, max_finish_time_ms, nopenmp_threads);
    return 0;
}
