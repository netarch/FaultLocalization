#include "bayesian_net.h"
#include "bayesian_net_continuous.h"
#include "bayesian_net_sherlock.h"
#include "doubleO7.h"
#include "net_bouncer.h"
#include "black_hole_utils.h"
#include "utils.h"
#include <assert.h>
#include <bits/stdc++.h>
#include <chrono>
#include <iostream>

using namespace std;

void LocalizeProbAnalysis(LogData &data,
                          map<PII, pair<int, double>> &failed_components,
                          double min_start_time_ms, double max_finish_time_ms,
                          int nopenmp_threads) {
    BayesianNet estimator;
    vector<double> params = {1.0 - 1.0e-3, 1.0e-4, -25.0};
    estimator.SetParams(params);

    // Partition for each (src, dst) pair
    map<PII, vector<Flow *>> endpoint_flow_map;
    for (Flow *flow : data.flows) {
        PII ep(flow->src, flow->dest);
        if (endpoint_flow_map.find(ep) == endpoint_flow_map.end()) {
            endpoint_flow_map[ep] = vector<Flow *>();
        }
        endpoint_flow_map[ep].push_back(flow);
    }

    LogData ep_data(data);
    for (auto const &[ep, ep_flows] : endpoint_flow_map) {
        ep_data.CleanupFlows();
        ep_data.flows = ep_flows;
        ep_data.failed_links.clear();
        int dropped_flows = 0;
        for (Flow *flow : ep_data.flows) {
            dropped_flows += (int)(flow->GetLatestPacketsLost() > 0);
        }
        if (dropped_flows) {
            Flow *flow = ep_data.flows[0];
            cout << flow->src << "(" << ep_data.hosts_to_racks[flow->src]
                 << ")->" << flow->dest << "("
                 << ep_data.hosts_to_racks[flow->dest]
                 << ") dropped flows:" << dropped_flows << endl;
            cout << "Printing paths::  " << endl;
            vector<Path *> *flow_paths = flow->GetPaths(max_finish_time_ms);
            Path device_path;
            for (Path *link_path : *flow_paths) {
                ep_data.GetDeviceLevelPath(flow, *link_path, device_path);
                for (int device : device_path) {
                    cout << " " << device;
                }
                cout << endl;
            }
            if (failed_components.find(ep) != failed_components.end()) {
                // pair<Link, double> failed_link = failed_components[ep];
                // ep_data.failed_links.insert(failed_link);
                // cout << "Failed link " << failed_link << "for pair " << ep <<
                // endl;
                pair<int, double> failed_device = failed_components[ep];
                ep_data.failed_devices.insert(failed_device);
                cout << "Failed device " << failed_device << " for pair " << ep
                     << endl;
            }
            Hypothesis failed_devices_set;
            ep_data.GetFailedDevices(failed_devices_set);
            estimator.SetLogData(&ep_data, max_finish_time_ms, nopenmp_threads);
            Hypothesis estimator_hypothesis;
            auto start_localization_time = chrono::high_resolution_clock::now();
            estimator.LocalizeDeviceFailures(
                min_start_time_ms, max_finish_time_ms, estimator_hypothesis,
                nopenmp_threads);
            PDD precision_recall = GetPrecisionRecall(
                failed_devices_set, estimator_hypothesis, &ep_data);
            cout << "Output Hypothesis: " << estimator_hypothesis
                 << " precsion_recall " << precision_recall.first << " "
                 << precision_recall.second << endl;
            cout << "Finished localization in "
                 << GetTimeSinceSeconds(start_localization_time) << " seconds"
                 << endl;
            cout << "****************************" << endl << endl << endl;
        }
    }
}

int main(int argc, char *argv[]) {
    assert(argc == 6);
    string trace_file(argv[1]);
    cout << "Running analysis on file " << trace_file << endl;
    string topology_file(argv[2]);
    cout << "Reading topology from file " << topology_file << endl;
    double min_start_time_ms = atof(argv[3]) * 1000.0,
           max_finish_time_ms = atof(argv[4]) * 1000.0;
    int nopenmp_threads = atoi(argv[5]);
    cout << "Using " << nopenmp_threads << " openmp threads" << endl;

    BayesianNet estimator;
    vector<double> params = {1.0 - 1.0e-3, 1.0e-4, -25.0};
    estimator.SetParams(params);
    PATH_KNOWN = false;
    TRACEROUTE_BAD_FLOWS = false;
    INPUT_FLOW_TYPE = APPLICATION_FLOWS;

    LogData data;
    cout << "calling GetDataFromLogFileParallel" << endl;
    GetDataFromLogFileParallel(trace_file, topology_file, &data,
                               nopenmp_threads);
    string fail_file = trace_file + ".fails";
    auto failed_components = ReadFailuresBlackHole(fail_file);

    LocalizeProbAnalysis(data, failed_components, min_start_time_ms,
                         max_finish_time_ms, nopenmp_threads);
    return 0;
}