#include "bayesian_net.h"
#include "bayesian_net_sherlock.h"
#include "common_testing_system.h"
#include "doubleO7.h"
#include "net_bouncer.h"
#include "score.h"
#include "utils.h"
#include <assert.h>
#include <chrono>
#include <iostream>

using namespace std;

bool USE_DIFFERENT_TOPOLOGIES = false;
bool CROSS_VALIDATION = true;
bool TRAINING_SET = false;
extern bool FLOW_DELAY;

vector<string> GetFilesLinkFlap() {
    string file_prefix =
        "/home/vharsh2/Flock/hw_traces/link_flap_traces/plog_skewed";
    vector<PII> ignore_files = {};
    vector<string> files;
    for (int f = 0; f <= 1; f++) {
        // for (int f: vector<int>({8})){ // {1-8}
        // for(int s: vector<int>({1,2,3,4,5,6,7,8})){ // {1-2}
        for (int s = 0; s < 25; s++) {
            if (CROSS_VALIDATION and (s + (int)TRAINING_SET) % 2 == 0)
                continue;
            if (find(ignore_files.begin(), ignore_files.end(), PII(f, s)) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" +
                                to_string(s));
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    ignore_files = {};
    file_prefix = "/home/vharsh2/Flock/hw_traces/link_flap_traces/plog_random";
    for (int f = 0; f <= 1; f++) {
        for (int s = 0; s < 25; s++) {
            if (CROSS_VALIDATION and (s + (int)TRAINING_SET) % 2 == 0)
                continue;
            if (find(ignore_files.begin(), ignore_files.end(), PII(f, s)) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" +
                                to_string(s));
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    return files;
}

vector<string> GetFilesPacketCorruption() {
    string base_dir = "/home/vharsh2//Flock/hw_traces/emulated";
    string file_prefix = base_dir + "/drop_prob_0.5/tor_uplink/test_random";
    vector<PII> ignore_files = {};
    vector<string> files;
    for (int f = 1; f <= 1; f++) {
        // for (int f: vector<int>({8})){ // {1-8}
        // for(int s: vector<int>({1,2,3,4,5,6,7,8})){ // {1-2}
        for (int s = 0; s < 30; s++) {
            if (CROSS_VALIDATION and (s + (int)TRAINING_SET) % 2 == 0)
                continue;
            if (find(ignore_files.begin(), ignore_files.end(), PII(f, s)) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_" + to_string(f) + "_" +
                                to_string(s));
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    ignore_files = {};
    file_prefix = base_dir + "/drop_prob_0.5/tor_uplink/test_skewed";
    for (int f = 1; f <= 1; f++) {
        for (int s = 0; s < 30; s++) {
            if (CROSS_VALIDATION and (s + (int)TRAINING_SET) % 2 == 0)
                continue;
            if (find(ignore_files.begin(), ignore_files.end(), PII(f, s)) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_" + to_string(f) + "_" +
                                to_string(s));
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    ignore_files = {};
    file_prefix = base_dir + "/drop_prob_0.5/host_uplink/test_random";
    for (int f = 1; f <= 1; f++) {
        for (int s = 0; s < 30; s++) {
            if (CROSS_VALIDATION and (s + (int)TRAINING_SET) % 2 == 0)
                continue;
            if (find(ignore_files.begin(), ignore_files.end(), PII(f, s)) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_" + to_string(f) + "_" +
                                to_string(s));
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    ignore_files = {};
    file_prefix = base_dir + "/drop_prob_0.5/host_uplink/test_skewed";
    for (int f = 1; f <= 1; f++) {
        for (int s = 0; s < 30; s++) {
            if (CROSS_VALIDATION and (s + (int)TRAINING_SET) % 2 == 0)
                continue;
            if (find(ignore_files.begin(), ignore_files.end(), PII(f, s)) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_" + to_string(f) + "_" +
                                to_string(s));
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    ignore_files = {};
    file_prefix = base_dir + "/no_failure/test_random";
    for (int f = 0; f <= 0; f++) {
        int smin =
            (CROSS_VALIDATION
                 ? 0
                 : 15); // if not cross validation, then hybrid calibration
        for (int s = smin; s < 30; s++) {
            if (CROSS_VALIDATION and (s + (int)TRAINING_SET) % 2 == 0)
                continue;
            if (find(ignore_files.begin(), ignore_files.end(), PII(f, s)) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_" + to_string(f) + "_" +
                                to_string(s));
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    ignore_files = {};
    file_prefix = base_dir + "/no_failure/test_skewed";
    for (int f = 0; f <= 0; f++) {
        int smin =
            (CROSS_VALIDATION
                 ? 0
                 : 15); // if not cross validation, then hybrid calibration
        for (int s = smin; s < 30; s++) {
            if (CROSS_VALIDATION and (s + (int)TRAINING_SET) % 2 == 0)
                continue;
            if (find(ignore_files.begin(), ignore_files.end(), PII(f, s)) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_" + to_string(f) + "_" +
                                to_string(s));
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    return files;
}

vector<string> GetFilesWred() {
    // string file_prefix =
    // "/home/vharsh2/Flock/ns3/topology/hw_ls_6_2/optical_fault/plog_testbed";
    // string file_prefix = "collector/logs/ls_6_2_minus_h39/t30s/plog_testbed";
    // string file_prefix = "collector/logs/ls_6_2/hw_logs/plog_testbed";
    vector<PII> ignore_files = {{1, 6}};
    vector<string> files;
    string file_prefix =
        "/home/vharsh2/Flock/hw_traces/wred_traces/plog_skewed";
    for (int f = 0; f <= 2; f++) {
        // for (int f: vector<int>({8})){ // {1-8}
        // for(int s: vector<int>({1,2,3,4,5,6,7,8})){ // {1-2}
        int smin = (f == 0 and !CROSS_VALIDATION ? 14 : 0);
        for (int s = smin; s < 30; s++) {
            if (CROSS_VALIDATION and (s + (int)TRAINING_SET) % 2 == 0)
                continue;
            if (find(ignore_files.begin(), ignore_files.end(), PII(f, s)) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" +
                                to_string(s));
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    ignore_files = {};
    file_prefix = "/home/vharsh2/Flock/hw_traces/wred_traces/plog_random";
    for (int f = 0; f <= 2; f++) {
        int smin = (f == 0 and !CROSS_VALIDATION ? 14 : 0);
        for (int s = smin; s < 30; s++) {
            if (CROSS_VALIDATION and (s + (int)TRAINING_SET) % 2 == 0)
                continue;
            if (find(ignore_files.begin(), ignore_files.end(), PII(f, s)) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" +
                                to_string(s));
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    return files;
}

pair<vector<string>, vector<string>> GetFilesHwCalibrationWred() {
    string file_prefix = "/home/vharsh2/Flock/ns3/topology/hw_ls_6_2/"
                         "calibration/wred_logs/plog_nb";
    string topology = "/home/vharsh2/Flock/ns3/topology/hw_ls_6_2/calibration/"
                      "ns3ls_x6_y2_i1.edgelist";
    vector<PII> ignore_files = {};
    vector<string> files, topologies;
    for (int f = 1; f <= 3; f++) {
        // for (int f: vector<int>({8})){ // {1-8}
        // for(int s: vector<int>({1,2,3,4,5,6,7,8})){ // {1-2}
        for (int s = 1; s < 25; s++) {
            if (find(ignore_files.begin(), ignore_files.end(), PII(f, s)) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" +
                                to_string(s));
                topologies.push_back(topology);
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    topology = "/home/vharsh2/Flock/hw_traces/wred_traces/ls_6_2.edgelist";
    ignore_files = {{1, 6}};
    file_prefix = "/home/vharsh2/Flock/hw_traces/wred_traces/plog_skewed";
    for (int f = 0; f <= 0; f++) {
        for (int s = 0; s < 14; s++) {
            if (find(ignore_files.begin(), ignore_files.end(), PII(f, s)) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" +
                                to_string(s));
                topologies.push_back(topology);
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    ignore_files = {};
    file_prefix = "/home/vharsh2/Flock/hw_traces/wred_traces/plog_random";
    for (int f = 0; f <= 0; f++) {
        for (int s = 0; s < 14; s++) {
            if (find(ignore_files.begin(), ignore_files.end(), PII(f, s)) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" +
                                to_string(s));
                topologies.push_back(topology);
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    return pair<vector<string>, vector<string>>(files, topologies);
}

pair<vector<string>, vector<string>> GetFilesHwCalibrationPacor() {
    string file_prefix = "/home/vharsh2/Flock/ns3/topology/hw_ls_6_2/"
                         "calibration/packet_corruption_logs/t60/plog_nb";
    string topology = "/home/vharsh2/Flock/ns3/topology/hw_ls_6_2/calibration/"
                      "ns3ls_x6_y2_i1.edgelist";
    vector<PII> ignore_files = {};
    vector<string> files, topologies;
    for (int f = 1; f <= 3; f++) {
        // for (int f: vector<int>({8})){ // {1-8}
        // for(int s: vector<int>({1,2,3,4,5,6,7,8})){ // {1-2}
        for (int s = 1; s < 25; s++) {
            if (find(ignore_files.begin(), ignore_files.end(), PII(f, s)) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" +
                                to_string(s));
                topologies.push_back(topology);
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    ignore_files = {};
    string base_dir = "/home/vharsh2//Flock/hw_traces/emulated";
    topology = base_dir + "/ls_6_2.edgelist";
    file_prefix = base_dir + "/no_failure/test_random";
    for (int f = 0; f <= 0; f++) {
        for (int s = 0; s < 15; s++) {
            if (find(ignore_files.begin(), ignore_files.end(), PII(f, s)) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_" + to_string(f) + "_" +
                                to_string(s));
                topologies.push_back(topology);
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    ignore_files = {};
    file_prefix = base_dir + "/no_failure/test_skewed";
    for (int f = 0; f <= 0; f++) {
        for (int s = 0; s < 15; s++) {
            if (find(ignore_files.begin(), ignore_files.end(), PII(f, s)) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_" + to_string(f) + "_" +
                                to_string(s));
                topologies.push_back(topology);
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    return pair<vector<string>, vector<string>>(files, topologies);
}

vector<string> GetFilesHwCalibration() {
    // string file_prefix =
    // "/home/vharsh2/flock/ns3/topology/hw_ls_6_2/optical_fault/plog_testbed";
    // string file_prefix =
    // "/home/vharsh2/Flock/ns3/topology/hw_ls_6_2/calibration/wred_logs/plog_nb";
    string file_prefix = "/home/vharsh2/Flock/ns3/topology/hw_ls_6_2/"
                         "calibration/packet_corruption_logs/t60/plog_nb";
    vector<PII> ignore_files = {};
    vector<string> files;
    for (int f = 1; f <= 3; f++) {
        // for (int f: vector<int>({8})){ // {1-8}
        // for(int s: vector<int>({1,2,3,4,5,6,7,8})){ // {1-2}
        for (int s = 1; s < 25; s++) {
            if (find(ignore_files.begin(), ignore_files.end(), PII(f, s)) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" +
                                to_string(s));
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    // files = {file_prefix + "_1_0_1"};
    return files;
}

vector<string> GetFilesHw() {
    // string file_prefix =
    // "/home/vharsh2/flock/ns3/topology/hw_ls_6_2/optical_fault/plog_testbed";
    string file_prefix = "/home/vharsh2/Flock/hw_traces/wred/plog";
    vector<PII> ignore_files = {};
    vector<string> files;
    for (int f = 0; f <= 2; f++) {
        // for (int f: vector<int>({8})){ // {1-8}
        // for(int s: vector<int>({1,2,3,4,5,6,7,8})){ // {1-2}
        for (int s = 0; s < 99; s++) {
            if (find(ignore_files.begin(), ignore_files.end(), PII(f, s)) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" +
                                to_string(s));
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    // files = {file_prefix + "_1_0_1"};
    return files;
}

vector<string> GetFilesDevice40G() {
    string file_dir =
        "/home/vharsh2/Flock/ns3/topology/ft_k10_os3/logs/40G/device";
    vector<PII> ignore_files = {};
    vector<string> files;
    for (string frac : vector<string>({"0.25", "0.5", "0.75", "1"})) {
        string file_prefix = file_dir + "/frac_" + frac + "/plog_nb";
        for (int f = 1; f <= 2; f++) {
            // for (int f: vector<int>({8})){ // {1-8}
            for (int s = 1; s <= 8; s++) {
                if (CROSS_VALIDATION and (s + (int)TRAINING_SET) % 2 == 0)
                    continue;
                if (find(ignore_files.begin(), ignore_files.end(), PII(f, s)) ==
                    ignore_files.end()) {
                    files.push_back(file_prefix + "_f" + to_string(f) + "_0_s" +
                                    to_string(s));
                    cout << "adding file for analaysis " << files.back()
                         << endl;
                }
            }
        }
    }
    return files;
}

pair<vector<string>, vector<string>> GetFilesMisconfiguredAclIrreg() {
    string file_prefix =
        "/home/vharsh2/Flock/flow_simulator/logs/irreg/plog_macl";
    string topo_prefix = "/home/vharsh2/Flock/flow_simulator/logs/irreg/"
                         "topologies/ns3ft_o100_deg10_sw125_svr250_os3";
    vector<PII> ignore_files = {};
    vector<string> files, topologies;
    for (int f = 1; f <= 2; f++) {
        // for (int f: vector<int>({8})){ // {1-8}
        for (int s = 1; s <= 32; s++) { // 1,2,3,4,5,6,7,8})){ // {1-2}
            if (CROSS_VALIDATION and (s + (int)TRAINING_SET) % 2 == 0)
                continue;
            if (find(ignore_files.begin(), ignore_files.end(), PII(f, s)) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_f" + to_string(f) + "_0_s" +
                                to_string(s));
                topologies.push_back(topo_prefix + "_fail" + to_string(f) +
                                     "_seed" + to_string(s) + ".edgelist");
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    return {files, topologies};
}

vector<string> GetFilesMisconfiguredAcl() {
    string file_prefix =
        "/home/vharsh2/Flock/flow_simulator/logs/macl/plog_macl";
    vector<PII> ignore_files = {};
    vector<string> files;
    for (int f = 1; f <= 2; f++) {
        // for (int f: vector<int>({8})){ // {1-8}
        for (int s = 1; s <= 16; s++) { // 1,2,3,4,5,6,7,8})){ // {1-2}
            if (CROSS_VALIDATION and (s + (int)TRAINING_SET) % 2 == 0)
                continue;
            if (find(ignore_files.begin(), ignore_files.end(), PII(f, s)) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_f" + to_string(f) + "_0_s" +
                                to_string(s));
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    return files;
}

vector<string> GetFilesMixed40G() {
    assert(CROSS_VALIDATION);
    string file_prefix =
        "/home/vharsh2/Flock/ns3/topology/ft_k10_os3/logs/40G/plog_nb";
    vector<PII> ignore_files = {PII(8, 3)};
    vector<string> files;
    for (int f = 1; f <= 8; f++) {
        // for (int f: vector<int>({8})){ // {1-8}
        for (int s : vector<int>(
                 {1, 2, 3, 4, 5, 6, 7, 8})) { // 1,2,3,4,5,6,7,8})){ // {1-2}
            if ((s + (int)TRAINING_SET) % 2 == 0)
                continue;
            if (find(ignore_files.begin(), ignore_files.end(), PII(f, s)) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_f" + to_string(f) + "_0_s" +
                                to_string(s));
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    return files;
}

vector<string> GetFilesMixed() {
    // string file_prefix =
    // "/home/vharsh2/Flock/ns3/topology/hw_ls_6_2/traffic_files/plog";
    string file_prefix = "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/ns3/"
                         "topology/ft_k10_os3/traffic_files_nb/plog_nb";
    // string file_prefix =
    // "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/ns3/topology/rrg_sw125_svr250_os3/logs/plog";
    vector<PII> ignore_files = {};
    vector<string> files;
    for (int f = 1; f <= 8; f++) {
        // for (int f: vector<int>({8})){ // {1-8}
        for (int s : vector<int>({1, 2})) { // 1,2,3,4,5,6,7,8})){ // {1-2}
            if (find(ignore_files.begin(), ignore_files.end(), PII(f, s)) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_" + to_string(f) + "_0_" +
                                to_string(s));
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    // files = {file_prefix + "_1_0_1"};
    return files;
}

// need to increase max path length for this experiment
int NLINKS_OMMITTED_IRREGULAR = 100;
pair<vector<string>, vector<string>> GetFilesRRG() {
    // string file_prefix =
    // "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/flow_simulator/logs/irregular_topology/plog";
    string file_prefix =
        "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/ns3/topology/ft_k10_os3/"
        "ommitted/logs/new/network_links_only/plog";
    string topo_prefix = "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/ns3/"
                         "topology/ft_k10_os3/ommitted/topologies/ns3ft_o" +
                         to_string(NLINKS_OMMITTED_IRREGULAR) +
                         "_deg10_sw125_svr250_os3";
    typedef array<int, 3> AI3;
    vector<AI3> ignore_files = {{10, 1, 2}, {10, 1, 5}, {10, 3, 3},
                                {15, 2, 5}, {15, 2, 3}, {15, 1, 8},
                                {15, 2, 1}, {15, 2, 2}, {15, 2, 4}};
    vector<string> files, topologies;
    // for(int s: vector<int>({1,7})){ // {1-2}
    for (int s : vector<int>({1, 2, 3, 4, 5, 6, 7, 8})) { // {1-2}
        // for(int s: vector<int>({4})){ // {1-2}
        for (int f = 1; f <= 4; f++) { //{1-8}
            if (CROSS_VALIDATION and (s + TRAINING_SET) % 2 == 0)
                continue;
            /*
            if(CROSS_VALIDATION){
                if (TRAINING_SET and s%3 > 0) continue;
                if (!TRAINING_SET and s%3 == 0) continue;
            }
            */
            if (find(ignore_files.begin(), ignore_files.end(),
                     AI3({NLINKS_OMMITTED_IRREGULAR, f, s})) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_o" +
                                to_string(NLINKS_OMMITTED_IRREGULAR) + "_" +
                                to_string(f) + "_0_" + to_string(s));
                topologies.push_back(topo_prefix + "_fail" + to_string(f) +
                                     "_seed" + to_string(s) + ".edgelist");
                cout << "adding file for analaysis " << files.back() << " "
                     << topologies.back() << endl;
            }
        }
    }
    return {files, topologies};
}

vector<string> GetFilesFlowSimulator() {
    string file_prefix = "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/"
                         "localization/flow_simulator/ft_logs/plog";
    vector<pair<string, int>> ignore_files = {};
    vector<string> files;
    vector<string> loss_rate_strings = {"0.001", "0.002", "0.004"};
    for (string &loss_rate_string : loss_rate_strings) {
        for (int i = 1; i <= 1; i++) {
            for (int f : {10}) {
                files.push_back(file_prefix + "_" + loss_rate_string + "_" +
                                to_string(f) + "_" + to_string(i));
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    vector<string> loss_rate_strings1 = {"0.003", "0.004"};
    for (string &loss_rate_string : loss_rate_strings1) {
        for (int i = 1; i <= 1; i++) {
            for (int f : {10}) {
                files.push_back(file_prefix + "_min" + loss_rate_string +
                                "_mixed_" + to_string(f) + "_" + to_string(i));
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    return files;
}

string SOFTNESS_FILE_PREFIX;
vector<string> GetFilesSoftness() {
    vector<string> loss_rate_strings;
    string file_prefix;
    vector<pair<string, int>> random_ignore_files, skewed_ignore_files,
        ignore_files;
    vector<string> files;

    random_ignore_files = {{"0.002", 39}, {"0.004", 18}, {"0.004", 19},
                           {"0.004", 8},  {"0.014", 21}, {"0.014", 24},
                           {"0.014", 28}, {"0.014", 29}};
    skewed_ignore_files = {
        {"0.014", 3}, {"0.004", 19}, {"0.004", 2}, {"0.004", 22}, {"0.004", 6}};
    if (SOFTNESS_FILE_PREFIX.find("random") != string::npos)
        ignore_files = random_ignore_files;
    else if (SOFTNESS_FILE_PREFIX.find("skewed") != string::npos)
        ignore_files = skewed_ignore_files;
    else
        assert(false);

    for (int i = 1; i <= 32; i++) {
        bool ignore = false;
        for (auto [slr, ig] : ignore_files) {
            if (i == ig and
                SOFTNESS_FILE_PREFIX.find("_" + slr + "_") != string::npos)
                ignore = true;
        }
        if (CROSS_VALIDATION and (i + TRAINING_SET) % 2 == 0)
            continue;
        if (!ignore) {
            files.push_back(SOFTNESS_FILE_PREFIX + "_" + to_string(i));
            cout << "adding file for analaysis " << files.back() << endl;
        }
    }
    // files = {files[0]};
    return files;
}

vector<string> GetFilesSoftnessAll() {
    // string file_prefix =
    // "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/topology/ft_k10_os3/softness_logs/random_traffic_logs/plog";
    vector<string> loss_rate_strings;
    string file_prefix;
    vector<pair<string, int>> ignore_files;
    vector<string> files;

    file_prefix = "/home/vharsh2/Flock/ns3/topology/ft_k10_os3/logs/40G/"
                  "softness/plog_nb_random";
    loss_rate_strings = {"0.002", "0.004", "0.006", "0.01", "0.014", "0.018"};
    // loss_rate_strings = {"0.01"};
    ignore_files = {{"0.002", 39}, {"0.004", 18}, {"0.004", 19}, {"0.004", 8},
                    {"0.014", 21}, {"0.014", 24}, {"0.014", 28}, {"0.014", 29}};
    for (string &loss_rate_string : loss_rate_strings) {
        for (int i = 1; i <= 16; i++) {
            if (find(ignore_files.begin(), ignore_files.end(),
                     pair<string, int>(loss_rate_string, i)) ==
                ignore_files.end()) {
                // files.push_back(file_prefix + "_" + loss_rate_string + "_" +
                // to_string(i)); cout << "adding file for analaysis "
                // <<files.back() << endl;
            }
        }
    }
    file_prefix = "/home/vharsh2/Flock/ns3/topology/ft_k10_os3/logs/40G/"
                  "softness/plog_nb_skewed";
    loss_rate_strings = {"0.002", "0.004", "0.006", "0.01", "0.014", "0.018"};
    ignore_files = {
        {"0.014", 3}, {"0.004", 19}, {"0.004", 2}, {"0.004", 22}, {"0.004", 6}};
    for (string &loss_rate_string : loss_rate_strings) {
        for (int i = 1; i <= 16; i++) {
            if (find(ignore_files.begin(), ignore_files.end(),
                     pair<string, int>(loss_rate_string, i)) ==
                ignore_files.end()) {
                files.push_back(file_prefix + "_" + loss_rate_string + "_" +
                                to_string(i));
                cout << "adding file for analaysis " << files.back() << endl;
            }
        }
    }
    return files;
}

vector<string> GetFiles007Verification() {
    string file_prefix =
        "/home/vharsh2/ns-allinone-3.24.1/ns-3.24.1/topology/"
        "ft_core10_pods2_agg8_tor20_hosts40/fail_network_links/plog";
    vector<pair<string, int>> ignore_files = {};
    vector<string> files;
    string loss_rate_string = "0.002";
    for (int i = 1; i <= 16; i++) {
        if (find(ignore_files.begin(), ignore_files.end(),
                 pair<string, int>(loss_rate_string, i)) ==
            ignore_files.end()) {
            files.push_back(file_prefix + "_1_" + loss_rate_string + "_" +
                            to_string(i));
            cout << "adding file for analaysis " << files.back() << endl;
        }
    }
    return files;
}

pair<vector<string>, vector<string>> GetFilesTopologiesHelper() {
    // return GetFilesRRG();
    // return GetFilesHwCalibrationWred();
    return GetFilesHwCalibrationPacor();
}

vector<string> GetFilesHelper() {
    // return GetFilesHwCalibration();
    // return GetFilesHw();
    // return GetFilesRRG();
    // return GetFilesMixed();
    // return GetFilesMixed40G();
    // return GetFilesDevice40G();
    // return GetFilesMisconfiguredAcl();
    // return GetFiles007Verification();
    // return GetFilesFlowSimulator();
    // return GetFilesSoftness();
    // return GetFilesWred();
    return GetFilesPacketCorruption();
}

vector<string> (*GetFiles)() = GetFilesHelper;
pair<vector<string>, vector<string>> (*GetFilesTopologies)() =
    GetFilesTopologiesHelper;

void GetPrecisionRecallTrendScore(string topology_filename,
                                  double min_start_time_ms,
                                  double max_finish_time_ms, double step_ms,
                                  int nopenmp_threads) {
    vector<PDD> result;
    Score estimator;
    vector<double> param = {1.0};
    estimator.SetParams(param);
    GetPrecisionRecallTrendFiles(topology_filename, min_start_time_ms,
                                 max_finish_time_ms, step_ms, result,
                                 &estimator, nopenmp_threads);
    int ctr = 0;
    for (double finish_time_ms = min_start_time_ms + step_ms;
         finish_time_ms <= max_finish_time_ms; finish_time_ms += step_ms) {
        cout << "Finish time (ms) " << finish_time_ms << " " << result[ctr++]
             << endl;
    }
}

void GetPrecisionRecallTrend007(string topology_filename,
                                double min_start_time_ms,
                                double max_finish_time_ms, double step_ms,
                                int nopenmp_threads) {
    vector<PDD> result;
    DoubleO7 estimator;
    vector<double> param = {0.003};
    estimator.SetParams(param);
    GetPrecisionRecallTrendFiles(topology_filename, min_start_time_ms,
                                 max_finish_time_ms, step_ms, result,
                                 &estimator, nopenmp_threads);
    int ctr = 0;
    for (double finish_time_ms = min_start_time_ms + step_ms;
         finish_time_ms <= max_finish_time_ms; finish_time_ms += step_ms) {
        cout << "Finish time (ms) " << finish_time_ms << " " << result[ctr++]
             << endl;
    }
}

void GetPrecisionRecallTrendSherlock(string topology_filename,
                                     double min_start_time_ms,
                                     double max_finish_time_ms, double step_ms,
                                     int nopenmp_threads) {
    vector<PDD> result;
    Sherlock estimator;
    vector<double> param = {1.0 - 5.0e-3, 2.0e-4, -25.0};
    estimator.SetParams(param);
    GetPrecisionRecallTrendFiles(topology_filename, min_start_time_ms,
                                 max_finish_time_ms, step_ms, result,
                                 &estimator, nopenmp_threads);
    int ctr = 0;
    for (double finish_time_ms = min_start_time_ms + step_ms;
         finish_time_ms <= max_finish_time_ms; finish_time_ms += step_ms) {
        cout << "Finish time (ms) " << finish_time_ms << " " << result[ctr++]
             << endl;
    }
}

void GetPrecisionRecallTrendBayesianNet(string topology_filename,
                                        double min_start_time_ms,
                                        double max_finish_time_ms,
                                        double step_ms, int nopenmp_threads) {
    vector<PDD> result;
    BayesianNet estimator;
    // vector<double> param = {1.0-5.0e-3, 2.0e-4, -25.0};
    // vector<double> param = {1.0-1e-2, 1e-4, -10.0}; //{1.0-10.0e-3, 8e-4,
    // -100.0};
    vector<double> param = {1.0 - 10.0e-3, 8e-4, -100.0};
    /*
    if (INPUT_FLOW_TYPE==PROBLEMATIC_FLOWS){
        estimator.PRIOR = -15.0;
        param = {1.0-5.0e-3, 4.0e-4};
    }
    */
    estimator.SetParams(param);
    GetPrecisionRecallTrendFiles(topology_filename, min_start_time_ms,
                                 max_finish_time_ms, step_ms, result,
                                 &estimator, nopenmp_threads);
    int ctr = 0;
    for (double finish_time_ms = min_start_time_ms + step_ms;
         finish_time_ms <= max_finish_time_ms; finish_time_ms += step_ms) {
        cout << "Finish time (ms) " << finish_time_ms << " " << result[ctr++]
             << endl;
    }
}

typedef vector<double> ParamType;

vector<ParamType> GetBayesianNetParams() {
    vector<ParamType> params;
    double eps = 1.0e-10;
    if (FLOW_DELAY) {
        for (double p1c = 1.0e-4; p1c <= 10.0e-4 + eps; p1c += 1.0e-4) {
            for (double p2 = 1.0e-5; p2 <= 10.0e-5 + eps; p2 += 1.0e-5) {
                if (p2 >= p1c - 0.5e-4)
                    continue;
                for (double nprior :
                     {2.5, 5.0, 7.5, 10.0, 12.5, 15.0, 20.0, 25.0, 50.0, 100.0})
                    params.push_back(vector<double>{1.0 - p1c, p2, -nprior});
            }
        }
    } else if (MISCONFIGURED_ACL) {
        for (double p1c = 0.125e-2; p1c <= 10.0e-2 + eps; p1c += 0.5e-2) {
            for (double p2 = 0.1e-4; p2 <= 2e-4 + eps; p2 += 0.1e-4) {
                if (p2 >= p1c - 0.5e-4)
                    continue;
                // for (double nprior: {250, 500, 750, 1000, 1250, 1500})
                for (double nprior : {2.5, 5.0, 10.0, 20.0, 100.0})
                    params.push_back(vector<double>{1.0 - p1c, p2, -nprior});
            }
        }
    } else if (INPUT_FLOW_TYPE == ACTIVE_FLOWS) {
        for (double p1c = 0.5e-3; p1c <= 10.0e-3 + eps; p1c += 1.0e-3) {
            // for (double p2 = 2.0e-4; p2 <=7.0e-4+eps; p2 += 0.5e-4){
            for (double p2 = 0.25e-4; p2 <= 8.0e-4 + eps; p2 += 0.5e-4) {
                if (p2 >= p1c - 0.5e-5)
                    continue;
                for (double nprior = 0.0; nprior <= 20.0 + eps;
                     nprior += 2.0) // priors don't have to be too high
                    params.push_back(vector<double>{1.0 - p1c, p2, -nprior});
            }
        }
    } else if (GetFiles == GetFilesMixed40G) {
        for (double p1c = 2.0e-3; p1c <= 10.0e-3 + eps; p1c += 1.0e-3) {
            for (double p2 = 1.0e-4; p2 <= 8.0e-4 + eps; p2 += 1.0e-4) {
                if (p2 >= p1c - 0.5e-4)
                    continue;
                // for (double nprior: {5.0, 10.0, 25.0, 50.0})
                for (double nprior : {5, 10, 20, 30, 50, 100, 200})
                    params.push_back(vector<double>{1.0 - p1c, p2, -nprior});
            }
        }
    } else {
        for (double p1c = 2.0e-3; p1c <= 10.0e-3 + eps; p1c += 1.0e-3) {
            for (double p2 = 1.0e-4; p2 <= 8.0e-4 + eps; p2 += 1.0e-4) {
                if (p2 >= p1c - 0.5e-4)
                    continue;
                // for (double nprior: {5.0, 10.0, 25.0, 50.0})
                for (double nprior : {5, 10, 15, 20, 25, 35, 45, 50, 100, 200})
                    params.push_back(vector<double>{1.0 - p1c, p2, -nprior});
            }
        }
    }
    return params;
}

vector<tuple<ParamType, PDD>> SweepParamsBayesianNet(string topology_filename,
                                                     double min_start_time_ms,
                                                     double max_finish_time_ms,
                                                     int nopenmp_threads) {
    vector<ParamType> params = GetBayesianNetParams();
    /*
    params = {};
    for (double nprior: {5, 10, 15, 20, 25, 35, 45, 50, 100, 200})
        params.push_back(vector<double> {0, 0, -nprior});
    //params = {{0, 0, -25}};
    params = {{1.0 - 3.0e-3, 2.0e-4, -100}};

    // for softness experiments/device
    if (INPUT_FLOW_TYPE == ALL_FLOWS and PATH_KNOWN) params = {{1.0 - 0.003,
    0.0002, -15.0}}; else if (INPUT_FLOW_TYPE == ALL_FLOWS and !PATH_KNOWN and
    TRACEROUTE_BAD_FLOWS) params = {{1.0 - 0.002, 0.0003, -15.0}}; else if
    (INPUT_FLOW_TYPE == ALL_FLOWS and !PATH_KNOWN and !TRACEROUTE_BAD_FLOWS)
    params = {{1.0 - 0.006, 0.0003, -5.0}}; else if (INPUT_FLOW_TYPE ==
    ACTIVE_FLOWS) params = {{1.0 - 0.0005, 0.000175, -4.0}}; else if
    (INPUT_FLOW_TYPE == PROBLEMATIC_FLOWS and !PATH_KNOWN) params = {{1.0 -
    0.007, 0.0003, -35.0}}; else assert(false);
    */

    vector<PDD> result;
    BayesianNet estimator;
    GetPrecisionRecallParamsFiles(topology_filename, min_start_time_ms,
                                  max_finish_time_ms, params, result,
                                  &estimator, nopenmp_threads);
    vector<tuple<ParamType, PDD>> ret;
    int ctr = 0;
    for (auto &param : params) {
        param[0] = 1.0 - param[0];
        auto p_r = result[ctr++];
        // if (p_r.first > 0.0 and p_r.second > 0.0)
        cout << param << " " << p_r << endl;
        ret.push_back(make_tuple(param, p_r));
    }
    return ret;
}

void SweepParamsScore(string topology_filename, double min_start_time_ms,
                      double max_finish_time_ms, int nopenmp_threads) {
    double min_fail_threshold = 1;  // 0.001;
    double max_fail_threshold = 16; // 0.0025;
    double step = 0.1;              // 0.00005;
    vector<vector<double>> params;
    for (double fail_threshold = min_fail_threshold;
         fail_threshold < max_fail_threshold; fail_threshold += step) {
        params.push_back(vector<double>{fail_threshold});
    }
    vector<PDD> result;
    Score estimator;
    GetPrecisionRecallParamsFiles(topology_filename, min_start_time_ms,
                                  max_finish_time_ms, params, result,
                                  &estimator, nopenmp_threads);
    int ctr = 0;
    for (auto &param : params) {
        cout << param << " " << result[ctr++] << endl;
    }
}

vector<tuple<ParamType, PDD>> SweepParams007(string topology_filename,
                                             double min_start_time_ms,
                                             double max_finish_time_ms,
                                             int nopenmp_threads) {
    double min_fail_threshold = 0.0001; // 0.00125; //1; //0.001;
    double max_fail_threshold = 0.30;   // 16; //0.0025;
    double step = 0.0002;               /// 0.1; //0.00005;
    vector<ParamType> params;
    for (double fail_threshold = min_fail_threshold;
         fail_threshold < max_fail_threshold; fail_threshold += step) {
        params.push_back(vector<double>{fail_threshold});
    }

    // params = {{0.0083}}; //skewed
    // params = {{0.0023}}; // random

    vector<PDD> result;
    DoubleO7 estimator;
    GetPrecisionRecallParamsFiles(topology_filename, min_start_time_ms,
                                  max_finish_time_ms, params, result,
                                  &estimator, nopenmp_threads);
    int ctr = 0;
    vector<tuple<ParamType, PDD>> res;
    for (auto &param : params) {
        auto p_r = result[ctr++];
        if (p_r.first > 0.0 or p_r.second > 0.0) {
            cout << param << " " << p_r << endl;
            res.push_back(make_tuple(param, p_r));
        }
    }
    return res;
}

vector<ParamType> GetParamsNetBouncer() {
    vector<ParamType> params;
    double eps = 1.0e-8;
    // link flap
    if (FLOW_DELAY) {
        for (double bad_device_threshold = 0.01; bad_device_threshold <= 0.42;
             bad_device_threshold += 0.10) {
            for (double regularize_c = 0.05; regularize_c <= 0.401;
                 regularize_c += 0.05) {
                // double fail_threshold_c = 30-4;
                for (double fail_threshold_c = 50e-4;
                     fail_threshold_c <= 200.0e-4;
                     fail_threshold_c += 10e-4) { // hw_pacor
                    params.push_back(vector<double>{
                        regularize_c, fail_threshold_c, bad_device_threshold});
                }
            }
        }
    } else if (MISCONFIGURED_ACL) {
        for (double bad_device_threshold = 0.01; bad_device_threshold <= 0.42;
             bad_device_threshold += 0.10) {
            for (double regularize_c = 0.0125; regularize_c <= 0.051;
                 regularize_c += 0.0125) {
                // double fail_threshold_c = 30-4;
                for (double fail_threshold_c = 1.0e-4;
                     fail_threshold_c <= 10.0e-4;
                     fail_threshold_c += 1.0e-4) { // hw_pacor
                    params.push_back(vector<double>{
                        regularize_c, fail_threshold_c, bad_device_threshold});
                }
            }
        }
    } else {
        params.push_back(vector<double>{0.001, 0.001, 0.075});
        /* hw
        for (double bad_device_threshold = 0.01; bad_device_threshold <= 0.52;
        bad_device_threshold += 0.10){ for (double regularize_c = 0.001;
        regularize_c <= 0.201; regularize_c += 0.025){
                //double fail_threshold_c = 30-4;
                for (double fail_threshold_c = 2.5e-4; fail_threshold_c
        <= 30.0e-4; fail_threshold_c += 2.5e-4){ params.push_back(vector<double>
        {regularize_c, fail_threshold_c, bad_device_threshold});
                }
            }
        }
        */
        /*
        for (double bad_device_threshold = 0.075; bad_device_threshold <= 0.21;
        bad_device_threshold += 0.05){
        //for (double bad_device_threshold = 0.01; bad_device_threshold <= 0.52;
        bad_device_threshold += 0.10){
            //for (double regularize_c = 0.025; regularize_c <= 0.101;
        regularize_c += 0.025){ for (double regularize_c = 0.001; regularize_c
        <= 0.025; regularize_c += 0.0025){
            //for (double regularize_c = 0.06; regularize_c <= 4.0; regularize_c
        += 0.15){ for (double fail_threshold_c = 0.5e-3; fail_threshold_c
        <= 10.0e-3-eps; fail_threshold_c += 0.5e-3){
                    params.push_back(vector<double> {regularize_c,
        fail_threshold_c, bad_device_threshold});
                }
            }
        }
        */
    }
    return params;
}

vector<tuple<ParamType, PDD>> SweepParamsNetBouncer(string topology_filename,
                                                    double min_start_time_ms,
                                                    double max_finish_time_ms,
                                                    int nopenmp_threads) {
    vector<ParamType> params = GetParamsNetBouncer();
    // softness/device
    /*
    if (INPUT_FLOW_TYPE == ALL_FLOWS and PATH_KNOWN) params = {{0.011, 0.002,
    0.075}}; else if (INPUT_FLOW_TYPE == ACTIVE_FLOWS) params = {{0.001, 0.007,
    0.125}}; else assert(false);
    */

    vector<PDD> result;
    NetBouncer estimator;
    GetPrecisionRecallParamsFiles(topology_filename, min_start_time_ms,
                                  max_finish_time_ms, params, result,
                                  &estimator, nopenmp_threads);
    int ctr = 0;
    vector<tuple<ParamType, PDD>> res;
    for (auto &param : params) {
        auto p_r = result[ctr++];
        cout << param << " " << p_r << endl;
        res.push_back(make_tuple(param, p_r));
    }
    return res;
}

void WriteResultsToFile(string outfile, vector<tuple<ParamType, PDD>> res) {
    ofstream of;
    of.open(outfile);
    for (auto &[param, p_r] : res) {
        of << param << " " << p_r << endl;
    }
    of.close();
    cout << "Written results to " << outfile << endl;
}

void SweepParamsSchemes(vector<string> &schemes, string topology_filename,
                        string base_dir, double min_start_time_ms,
                        double max_finish_time_ms, int nopenmp_threads) {
    vector<tuple<ParamType, PDD>> res;
    for (string scheme : schemes) {
        if (scheme == "bnet_int") {
            PATH_KNOWN = true;
            INPUT_FLOW_TYPE = ALL_FLOWS;
            res = SweepParamsBayesianNet(topology_filename, min_start_time_ms,
                                         max_finish_time_ms, nopenmp_threads);
        } else if (scheme == "bnet_a1_a2_p") {
            PATH_KNOWN = false;
            INPUT_FLOW_TYPE = ALL_FLOWS;
            TRACEROUTE_BAD_FLOWS = true;
            res = SweepParamsBayesianNet(topology_filename, min_start_time_ms,
                                         max_finish_time_ms, nopenmp_threads);
        } else if (scheme == "bnet_a2_p") {
            PATH_KNOWN = false;
            INPUT_FLOW_TYPE = APPLICATION_FLOWS;
            TRACEROUTE_BAD_FLOWS = true;
            res = SweepParamsBayesianNet(topology_filename, min_start_time_ms,
                                         max_finish_time_ms, nopenmp_threads);
        } else if (scheme == "bnet_a2") {
            PATH_KNOWN = false;
            INPUT_FLOW_TYPE = PROBLEMATIC_FLOWS;
            TRACEROUTE_BAD_FLOWS = true;
            res = SweepParamsBayesianNet(topology_filename, min_start_time_ms,
                                         max_finish_time_ms, nopenmp_threads);
        } else if (scheme == "bnet_a1_p") {
            PATH_KNOWN = false;
            INPUT_FLOW_TYPE = ALL_FLOWS;
            TRACEROUTE_BAD_FLOWS = false;
            res = SweepParamsBayesianNet(topology_filename, min_start_time_ms,
                                         max_finish_time_ms, nopenmp_threads);
        } else if (scheme == "bnet_a1") {
            PATH_KNOWN = false;
            INPUT_FLOW_TYPE = ACTIVE_FLOWS;
            res = SweepParamsBayesianNet(topology_filename, min_start_time_ms,
                                         max_finish_time_ms, nopenmp_threads);
        } else if (scheme == "bnet_p") {
            PATH_KNOWN = false;
            INPUT_FLOW_TYPE = APPLICATION_FLOWS;
            TRACEROUTE_BAD_FLOWS = false;
            res = SweepParamsBayesianNet(topology_filename, min_start_time_ms,
                                         max_finish_time_ms, nopenmp_threads);
        } else if (scheme == "nb_a1") {
            PATH_KNOWN = false;
            INPUT_FLOW_TYPE = ACTIVE_FLOWS;
            res = SweepParamsNetBouncer(topology_filename, min_start_time_ms,
                                        max_finish_time_ms, nopenmp_threads);
        } else if (scheme == "nb_int") {
            PATH_KNOWN = true;
            INPUT_FLOW_TYPE = ALL_FLOWS;
            res = SweepParamsNetBouncer(topology_filename, min_start_time_ms,
                                        max_finish_time_ms, nopenmp_threads);
        } else if (scheme == "007_a2") {
            PATH_KNOWN = false;
            INPUT_FLOW_TYPE = PROBLEMATIC_FLOWS;
            TRACEROUTE_BAD_FLOWS = true;
            res = SweepParams007(topology_filename, min_start_time_ms,
                                 max_finish_time_ms, nopenmp_threads);
        } else {
            cout << "Unknown scheme " << scheme << endl;
            assert(false);
        }
        string outfile = base_dir + scheme;
        WriteResultsToFile(outfile, res);
    }
}

void SetTestbedScenario(string scenario) {
    if (scenario == "link_flap") {
        GetFiles = GetFilesLinkFlap;
        FLOW_DELAY = true;
    } else if (scenario == "wred") {
        GetFiles = GetFilesWred;
        GetFilesTopologies = GetFilesHwCalibrationWred;
    } else if (scenario == "pacor") {
        GetFiles = GetFilesPacketCorruption;
        GetFilesTopologies = GetFilesHwCalibrationPacor;
    } else
        assert(false);
}

void TestbedCrossValidataion(string topology_filename, double min_start_time_ms,
                             double max_finish_time_ms, int nopenmp_threads) {
    USE_DIFFERENT_TOPOLOGIES = false;
    CROSS_VALIDATION = true;
    TRAINING_SET = true;

    string scenario = "wred";
    SetTestbedScenario(scenario);

    if (scenario == "pacor")
        scenario += "/remove_bad_flow";

    // vector<string> schemes = {"bnet_int", "bnet_a2_p", "bnet_a2", "nb_int",
    // "007_a2"};
    vector<string> schemes = {"nb_int"};

    string base_dir = "/home/vharsh2/Flock/localization/c++/results/hw/" +
                      scenario + "/train_";
    SweepParamsSchemes(schemes, topology_filename, base_dir, min_start_time_ms,
                       max_finish_time_ms, nopenmp_threads);

    TRAINING_SET = false;
    base_dir = "/home/vharsh2/Flock/localization/c++/results/hw/" + scenario +
               "/test_";
    SweepParamsSchemes(schemes, topology_filename, base_dir, min_start_time_ms,
                       max_finish_time_ms, nopenmp_threads);
}

void TestbedHybridCalibration(string topology_filename,
                              double min_start_time_ms,
                              double max_finish_time_ms, int nopenmp_threads) {
    CROSS_VALIDATION = false;
    USE_DIFFERENT_TOPOLOGIES = true;

    string scenario = "pacor";
    SetTestbedScenario(scenario);

    if (scenario == "pacor")
        scenario += "/remove_bad_flow";

    vector<string> schemes = {"bnet_int", "bnet_a2_p", "bnet_a2", "nb_int",
                              "007_a2"};

    string base_dir =
        "/home/vharsh2/Flock/localization/c++/results/conext2023/hw/" +
        scenario + "/cal_";
    SweepParamsSchemes(schemes, topology_filename, base_dir, min_start_time_ms,
                       max_finish_time_ms, nopenmp_threads);

    USE_DIFFERENT_TOPOLOGIES = false;
    base_dir = "/home/vharsh2/Flock/localization/c++/results/conext2023/hw/" +
               scenario + "/cal_hw_";
    SweepParamsSchemes(schemes, topology_filename, base_dir, min_start_time_ms,
                       max_finish_time_ms, nopenmp_threads);
}

void SimulatorMixedTracesExperiments(string topology_filename,
                                     double min_start_time_ms,
                                     double max_finish_time_ms,
                                     int nopenmp_threads) {
    USE_DIFFERENT_TOPOLOGIES = false;
    CROSS_VALIDATION = true;
    GetFiles = GetFilesMixed40G;

    TRAINING_SET = true;
    string monitoring_period = "tdot5";
    if (max_finish_time_ms - min_start_time_ms > 1000.0)
        monitoring_period = "t1";
    else if (max_finish_time_ms - min_start_time_ms > 500.0)
        monitoring_period = "tdot5";
    else if (max_finish_time_ms - min_start_time_ms > 250.0)
        monitoring_period = "tdot25";
    cout << "Monitoring period " << monitoring_period << endl;

    string base_dir =
        "/home/vharsh2/Flock/localization/c++/results/conext2023/mixed/" +
        monitoring_period + "_train_";
    // vector<string> schemes = {"bnet_int", "bnet_a1_a2_p", "bnet_a2",
    // "bnet_a1_p", "bnet_a1", "nb_int", "nb_a1", "007_a2"};
    vector<string> schemes = {"bnet_int", "bnet_a1_a2_p", "bnet_a2",
                              "bnet_a1_p", "bnet_a1"};
    // vector<string> schemes = {};
    cout << "Training for " << schemes << endl;
    SweepParamsSchemes(schemes, topology_filename, base_dir, min_start_time_ms,
                       max_finish_time_ms, nopenmp_threads);

    TRAINING_SET = false;
    base_dir =
        "/home/vharsh2/Flock/localization/c++/results/conext2023/mixed/" +
        monitoring_period + "_test_";
    // schemes = {"bnet_int", "bnet_a1"};
    cout << "Testing for " << schemes << endl;
    SweepParamsSchemes(schemes, topology_filename, base_dir, min_start_time_ms,
                       max_finish_time_ms, nopenmp_threads);
}

void SimulatorMisconfiguredAclIrreg(string topology_filename,
                                    double min_start_time_ms,
                                    double max_finish_time_ms,
                                    int nopenmp_threads) {
    USE_DIFFERENT_TOPOLOGIES = true;
    CROSS_VALIDATION = true;
    MISCONFIGURED_ACL = true;
    GetFilesTopologies = GetFilesMisconfiguredAclIrreg;

    // vector<string> schemes = {"bnet_p", "nb_int", "007_a2"};
    vector<string> schemes = {"bnet_p"};

    TRAINING_SET = true;
    string base_dir = "/home/vharsh2/Flock/localization/c++/results/conext2023/"
                      "macl/irreg/train_";
    SweepParamsSchemes(schemes, topology_filename, base_dir, min_start_time_ms,
                       max_finish_time_ms, nopenmp_threads);

    TRAINING_SET = false;
    base_dir = "/home/vharsh2/Flock/localization/c++/results/conext2023/macl/"
               "irreg/test_";
    SweepParamsSchemes(schemes, topology_filename, base_dir, min_start_time_ms,
                       max_finish_time_ms, nopenmp_threads);
}

void SimulatorMisconfiguredAcl(string topology_filename,
                               double min_start_time_ms,
                               double max_finish_time_ms, int nopenmp_threads) {
    USE_DIFFERENT_TOPOLOGIES = false;
    CROSS_VALIDATION = true;
    MISCONFIGURED_ACL = true;
    GetFiles = GetFilesMisconfiguredAcl;

    // vector<string> schemes = {"bnet_int", "bnet_a1_a2_p", "bnet_a2",
    // "bnet_a1_p", "bnet_a1", "nb_int", "nb_a1", "007_a2"};
    vector<string> schemes = {"bnet_p"};

    TRAINING_SET = true;
    string base_dir =
        "/home/vharsh2/Flock/localization/c++/results/conext2023/macl/train_";
    SweepParamsSchemes(schemes, topology_filename, base_dir, min_start_time_ms,
                       max_finish_time_ms, nopenmp_threads);

    TRAINING_SET = false;
    base_dir =
        "/home/vharsh2/Flock/localization/c++/results/conext2023/macl/test_";
    SweepParamsSchemes(schemes, topology_filename, base_dir, min_start_time_ms,
                       max_finish_time_ms, nopenmp_threads);
}

void SimulatorDeviceExperiments(string topology_filename,
                                double min_start_time_ms,
                                double max_finish_time_ms,
                                int nopenmp_threads) {
    USE_DIFFERENT_TOPOLOGIES = false;
    CROSS_VALIDATION = true;
    GetFiles = GetFilesDevice40G;
    DEVICE_ANALYSIS_007 = true;

    TRAINING_SET = true;

    string monitoring_period = "tdot5";
    // assert (max_finish_time_ms - min_start_time_ms > 450.0 and
    // max_finish_time_ms - min_start_time_ms < 550.0);
    if (max_finish_time_ms - min_start_time_ms > 1000.0)
        monitoring_period = "t1";
    else if (max_finish_time_ms - min_start_time_ms > 500.0)
        monitoring_period = "tdot5";
    else if (max_finish_time_ms - min_start_time_ms > 250.0)
        monitoring_period = "tdot25";
    cout << "Monitoring period " << monitoring_period << endl;

    // vector<string> schemes = {"nb_int", "nb_a1"};
    vector<string> schemes = {"007_a2"};
    cout << "(D) Training for " << schemes << endl;
    string base_dir =
        "/home/vharsh2/Flock/localization/c++/results/conext2023/device/" +
        monitoring_period + "_train_";
    SweepParamsSchemes(schemes, topology_filename, base_dir, min_start_time_ms,
                       max_finish_time_ms, nopenmp_threads);

    TRAINING_SET = false;
    base_dir =
        "/home/vharsh2/Flock/localization/c++/results/conext2023/device/" +
        monitoring_period + "_test_";
    schemes = {};
    cout << "(D) Testing for " << schemes << endl;
    SweepParamsSchemes(schemes, topology_filename, base_dir, min_start_time_ms,
                       max_finish_time_ms, nopenmp_threads);

    DEVICE_ANALYSIS_007 = false;
}

void SoftnessExperiments(string topology_filename, double min_start_time_ms,
                         double max_finish_time_ms, int nopenmp_threads) {
    CROSS_VALIDATION = false;

    string file_prefix, base_dir;
    vector<pair<string, int>> ignore_files;
    GetFiles = GetFilesSoftness;

    // vector<string> schemes = {"bnet_int", "bnet_a1_a2_p", "bnet_a2",
    // "bnet_a1_p", "bnet_a1", "nb_int", "nb_a1"}; vector<string> schemes =
    // {"bnet_int", "bnet_a1_a2_p", "bnet_a2", "bnet_a1_p", "bnet_a1"};
    vector<string> schemes = {"bnet_a2", "007_a2", "bnet_int", "nb_int"};

    file_prefix = "/home/vharsh2/Flock/ns3/topology/ft_k10_os3/logs/40G/"
                  "softness/plog_nb_random";
    // vector<string> loss_rate_strings = {"0.002", "0.004", "0.006", "0.01",
    // "0.014", "0.018"};

    vector<string> loss_rate_strings = {"0.018", "0.004"};
    CROSS_VALIDATION = true;
    for (string &loss_rate_string : loss_rate_strings) {
        SOFTNESS_FILE_PREFIX = file_prefix + "_" + loss_rate_string;
        if (CROSS_VALIDATION) {
            base_dir = "/home/vharsh2/Flock/localization/c++/results/"
                       "conext2023/softness/train_random_" +
                       loss_rate_string + "_";
            TRAINING_SET = true;
            SweepParamsSchemes(schemes, topology_filename, base_dir,
                               min_start_time_ms, max_finish_time_ms,
                               nopenmp_threads);
            base_dir = "/home/vharsh2/Flock/localization/c++/results/"
                       "conext2023/softness/test_random_" +
                       loss_rate_string + "_";
            TRAINING_SET = false;
            SweepParamsSchemes(schemes, topology_filename, base_dir,
                               min_start_time_ms, max_finish_time_ms,
                               nopenmp_threads);
        } else {
            base_dir = "/home/vharsh2/Flock/localization/c++/results/"
                       "conext2023/softness/random_" +
                       loss_rate_string + "_";
            SweepParamsSchemes(schemes, topology_filename, base_dir,
                               min_start_time_ms, max_finish_time_ms,
                               nopenmp_threads);
        }
    }

    return;

    file_prefix = "/home/vharsh2/Flock/ns3/topology/ft_k10_os3/logs/40G/"
                  "softness/plog_nb_skewed";
    loss_rate_strings = {"0.002", "0.004", "0.006", "0.01", "0.014", "0.018"};
    for (string &loss_rate_string : loss_rate_strings) {
        SOFTNESS_FILE_PREFIX = file_prefix + "_" + loss_rate_string;
        if (CROSS_VALIDATION) {
            base_dir = "/home/vharsh2/Flock/localization/c++/results/"
                       "conext2023/softness/train_skewed_" +
                       loss_rate_string + "_";
            TRAINING_SET = true;
            SweepParamsSchemes(schemes, topology_filename, base_dir,
                               min_start_time_ms, max_finish_time_ms,
                               nopenmp_threads);
            base_dir = "/home/vharsh2/Flock/localization/c++/results/"
                       "conext2023/softness/test_skewed_" +
                       loss_rate_string + "_";
            TRAINING_SET = false;
            SweepParamsSchemes(schemes, topology_filename, base_dir,
                               min_start_time_ms, max_finish_time_ms,
                               nopenmp_threads);
        } else {
            base_dir = "/home/vharsh2/Flock/localization/c++/results/"
                       "conext2023/softness/skewed_" +
                       loss_rate_string + "_";
            SweepParamsSchemes(schemes, topology_filename, base_dir,
                               min_start_time_ms, max_finish_time_ms,
                               nopenmp_threads);
        }
    }
}

int GetOmitLinksFromTopologyFilename(string topology_filename) {
    return stoi(topology_filename);
}

void IrregularTopology(string topology_filename, double min_start_time_ms,
                       double max_finish_time_ms, int nopenmp_threads) {
    CROSS_VALIDATION = true;
    USE_DIFFERENT_TOPOLOGIES = true;
    GetFilesTopologies = GetFilesRRG;

    TRAINING_SET = true;
    vector<string> schemes = {"bnet_p"}; //{"bnet_int", "bnet_a2_p", "bnet_a2",
                                         //"bnet_p", "nb_int", "007_a2"};
    // vector<int> nomit_links = {10}; //{0, 3, 6, 9, 12};
    vector<int> nomit_links = {
        GetOmitLinksFromTopologyFilename(topology_filename)};
    cout << "nomit_links: " << nomit_links << endl;
    // nomit_links = {0, 25, 50, 75, 100};
    for (int nomit : nomit_links) {
        NLINKS_OMMITTED_IRREGULAR = nomit;
        string base_dir = "/home/vharsh2/Flock/localization/c++/results/"
                          "conext2023/irregular/train_" +
                          to_string(NLINKS_OMMITTED_IRREGULAR) + "_";
        SweepParamsSchemes(schemes, topology_filename, base_dir,
                           min_start_time_ms, max_finish_time_ms,
                           nopenmp_threads);
    }

    TRAINING_SET = false;
    for (int nomit : nomit_links) {
        NLINKS_OMMITTED_IRREGULAR = nomit;
        string base_dir = "/home/vharsh2/Flock/localization/c++/results/"
                          "conext2023/irregular/test_" +
                          to_string(NLINKS_OMMITTED_IRREGULAR) + "_";
        // string base_dir =
        // "/home/vharsh2/Flock/localization/c++/results/asymmetric_clos/test_"
        // + to_string(NLINKS_OMMITTED_IRREGULAR) + "_";
        SweepParamsSchemes(schemes, topology_filename, base_dir,
                           min_start_time_ms, max_finish_time_ms,
                           nopenmp_threads);
    }
}

int main(int argc, char *argv[]) {
    VERBOSE = false;
    assert(argc == 6);
    string topology_filename(argv[1]);
    cout << "Reading topology from file " << topology_filename << endl;
    double min_start_time_ms = atof(argv[2]) * 1000.0,
           max_finish_time_ms = atof(argv[3]) * 1000.0;
    double step_ms = atof(argv[4]) * 1000.0;
    int nopenmp_threads = atoi(argv[5]);
    cout << "Using " << nopenmp_threads << " openmp threads" << endl;
    cout << "sizeof(Flow) " << sizeof(Flow) << " bytes" << endl;
    cout << "Analysis from time(ms) " << min_start_time_ms << " --> "
         << max_finish_time_ms << endl;
    GetFiles = GetFilesMixed;
    INPUT_FLOW_TYPE = ALL_FLOWS;
    TRACEROUTE_BAD_FLOWS = true;
    PATH_KNOWN = false;
    // GetPrecisionRecallTrendBayesianNet(topology_filename, min_start_time_ms,
    //                                   max_finish_time_ms, step_ms,
    //                                   nopenmp_threads);
    // SweepParamsBayesianNet(topology_filename, min_start_time_ms,
    // max_finish_time_ms, nopenmp_threads); SweepParams007(topology_filename,
    // min_start_time_ms, max_finish_time_ms, nopenmp_threads);
    // SweepParamsNetBouncer(topology_filename, min_start_time_ms,
    // max_finish_time_ms, nopenmp_threads);
    TestbedCrossValidataion(topology_filename, min_start_time_ms,
                            max_finish_time_ms, nopenmp_threads);
    // TestbedHybridCalibration(topology_filename, min_start_time_ms,
    // max_finish_time_ms, nopenmp_threads);
    // SimulatorMixedTracesExperiments(topology_filename, min_start_time_ms,
    // max_finish_time_ms, nopenmp_threads);
    // SoftnessExperiments(topology_filename, min_start_time_ms,
    // max_finish_time_ms, nopenmp_threads);
    // SimulatorDeviceExperiments(topology_filename, min_start_time_ms,
    // max_finish_time_ms, nopenmp_threads);
    // IrregularTopology(topology_filename, min_start_time_ms,
    // max_finish_time_ms, nopenmp_threads);
    // SimulatorMisconfiguredAcl(topology_filename, min_start_time_ms,
    // max_finish_time_ms, nopenmp_threads);
    // SimulatorMisconfiguredAclIrreg(topology_filename, min_start_time_ms,
    // max_finish_time_ms, nopenmp_threads);
    return 0;
}
