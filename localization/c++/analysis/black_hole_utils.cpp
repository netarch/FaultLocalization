#include "bayesian_net.h"
#include "bayesian_net_continuous.h"
#include "bayesian_net_sherlock.h"
#include "doubleO7.h"
#include "net_bouncer.h"
#include "utils.h"
#include "black_hole_utils.h"
#include <assert.h>
#include <bits/stdc++.h>
#include <chrono>
#include <iostream>

using namespace std;

void GetDroppedFlows(LogData &data, vector<Flow *> &dropped_flows) {
    for (Flow *flow : data.flows) {
        if (flow->GetLatestPacketsLost())
            dropped_flows.push_back(flow);
    }
}

// Check if removing a link does not eliminate all shortest paths
void CheckShortestPathExists(LogData &data, double max_finish_time_ms,
                             vector<Flow *> &dropped_flows,
                             int link_to_remove) {
    Hypothesis removed_links;
    removed_links.insert(link_to_remove);
    int rlink_id = data.GetReverseLinkId(link_to_remove);
    removed_links.insert(rlink_id);
    for (Flow *flow : dropped_flows) {
        vector<Path *> *flow_paths = flow->GetPaths(max_finish_time_ms);
        int npaths = 0;
        for (Path *link_path : *flow_paths) {
            if (!HypothesisIntersectsPath(&removed_links, link_path)) {
                npaths++;
            }
        }
        assert(npaths > 0);
    }
}

// ... in the modified network
void BinFlowsByDevice(LogData &data, double max_finish_time_ms,
                      vector<Flow *> &dropped_flows, Hypothesis &removed_links,
                      map<int, set<Flow *>> &flows_by_device) {
    for (Flow *flow : dropped_flows) {
        vector<Path *> *flow_paths = flow->GetPaths(max_finish_time_ms);
        for (Path *link_path : *flow_paths) {
            if (!HypothesisIntersectsPath(&removed_links, link_path)) {
                Path device_path;
                data.GetDeviceLevelPath(flow, *link_path, device_path);
                for (int device : device_path)
                    flows_by_device[device].insert(flow);
            }
        }
        cout << "Dropped flow: " << flow->src << "("
             << data.hosts_to_racks[flow->src] << ")->" << flow->dest << "("
             << data.hosts_to_racks[flow->dest] << ") " << endl;
    }
}

int GetExplanationEdges(LogData &data, double max_finish_time_ms,
                        vector<Flow *> &dropped_flows,
                        Hypothesis &removed_links, Hypothesis &result) {
    map<int, set<Flow *>> flows_by_device;
    BinFlowsByDevice(data, max_finish_time_ms, dropped_flows, removed_links,
                     flows_by_device);
    cout << "Num dropped flows " << dropped_flows.size() << endl;
    return GetExplanationEdgesFromMap(flows_by_device);
}

int GetExplanationEdgesFromMap(map<int, set<Flow *>> &flows_by_device){
    vector<pair<int, set<Flow *>>> sorted_map(flows_by_device.begin(),
                                              flows_by_device.end());
    sort(sorted_map.begin(), sorted_map.end(), SortByValueSize);
    int ntotal = 0;
    for (auto &[device, flows] : sorted_map) {
        cout << "Device " << device << " explains " << flows.size() << " drops"
             << endl;
        ntotal += flows.size();
    }
    cout << "Total explanation edges " << ntotal << endl;
    return ntotal;
}

map<int, set<Flow *>> BinFlowsByDeviceAgg(LogData *data,
                         vector<Flow*> *dropped_flows,
                         int ntraces,
                         set<Link> removed_links,
                         double max_finish_time_ms, int nopenmp_threads) {
    map<int, set<Flow *>> flows_by_device;
    for(int ii=0; ii < ntraces; ii++){
        Hypothesis removed_links_hyp;
        for (Link l: removed_links){
            if (data[ii].links_to_ids.find(l) != data[ii].links_to_ids.end())
                removed_links_hyp.insert(data[ii].links_to_ids[l]);
        }
        BinFlowsByDevice(data[ii], max_finish_time_ms, dropped_flows[ii], removed_links_hyp,
                         flows_by_device);
    }
    return flows_by_device;
}

set<int> GetEquivalentDevices(map<int, set<Flow*>> &flows_by_device){
    set<int> ret;
    vector<pair<int, set<Flow *>>> sorted_map(flows_by_device.begin(),
                                              flows_by_device.end());
    sort(sorted_map.begin(), sorted_map.end(), SortByValueSize);
    int maxedges = sorted_map[0].second.size();
    int ntotal = 0;
    for (auto &[device, flows] : sorted_map) {
        if (flows.size() < maxedges) break;
        cout << "Eq Device " << device << " explains " << flows.size() << " drops"
             << endl;
        ret.insert(device);
    }
    return ret;
}

void LocalizeScoreAgg(vector<pair<string, string>> &in_topo_traces,
                  double max_finish_time_ms, int nopenmp_threads) {
    int ntraces = in_topo_traces.size();
    vector<Flow *> dropped_flows[ntraces];
    LogData data[ntraces];
    for (int ii=0; ii < ntraces; ii++){
        string topo_f = in_topo_traces[ii].first;
        string trace_f = in_topo_traces[ii].second;
        cout << "calling GetDataFromLogFileParallel on " << topo_f << ", " << trace_f << endl;
        GetDataFromLogFileParallel(trace_f, topo_f, &data[ii], nopenmp_threads);
        GetDroppedFlows(data[ii], dropped_flows[ii]);
    }

    set<Link> removed_links;
    auto flows_by_device_agg = BinFlowsByDeviceAgg(data, dropped_flows, ntraces, removed_links, max_finish_time_ms, nopenmp_threads);
    set<int> equivalent_devices = GetEquivalentDevices(flows_by_device_agg);
    cout << "equivalent devices " << equivalent_devices << endl;

    GetExplanationEdgesFromMap(flows_by_device_agg);
    cout << "Explaining drops" << endl;

    //TODO: remove
    return;

    int old_expl_edges = int(1.0e9), curr_expl_edges = int(1.0e9);
    int max_iter = 10;
    while (1) {
        auto [best_link_to_remove, expl_edges] = GetBestLinkToRemoveAgg(data, dropped_flows, ntraces, equivalent_devices, removed_links,
                         max_finish_time_ms, nopenmp_threads);
        old_expl_edges = curr_expl_edges;
        curr_expl_edges = expl_edges;
        cout << "Best link to remove "
             << best_link_to_remove
             << " nexplanation edges " << expl_edges 
             << " removed_links " << removed_links << endl;
        if (old_expl_edges - curr_expl_edges < 10 or max_iter == 0)
            break;
        max_iter--;
        removed_links.insert(best_link_to_remove);
        Link rlink (best_link_to_remove.second, best_link_to_remove.first);
        removed_links.insert(rlink);
    }
}

pair<Link, int> GetBestLinkToRemoveAgg(LogData *data, vector<Flow*> *dropped_flows, int ntraces,
                                       set<int> &equivalent_devices,
                                       set<Link> &prev_removed_links,
                                       double max_finish_time_ms, int nopenmp_threads) {
    cout << "Equivalent devices " << equivalent_devices << endl;
    set<Link> removed_links;
    int total_expl_edges = GetExplanationEdgesAgg(
        data, dropped_flows, ntraces, equivalent_devices, removed_links, max_finish_time_ms);
    int min_expl_edges = int(1e9);
    Link best_link_to_remove = Link(-1, -1);
    for (Link link: data[0].inverse_links) {
        int link_id = data[0].links_to_ids[link];
        Link rlink = Link(link.second, link.first);
        if (data[0].IsNodeSwitch(link.first) and data[0].IsNodeSwitch(link.second) and
            !data[0].IsLinkDevice(link_id)) {
            removed_links = prev_removed_links;
            removed_links.insert(link);
            removed_links.insert(rlink);
            cout << "Removing link " << link << endl;
            for (int ii=0; ii < ntraces; ii++){
                int link_id_ii = data[ii].links_to_ids[link];
                CheckShortestPathExists(data[ii], max_finish_time_ms, dropped_flows[ii],
                                        link_id_ii);
            }

            int expl_edges = GetExplanationEdgesAgg(
                data, dropped_flows, ntraces, equivalent_devices, removed_links, max_finish_time_ms);
            expl_edges = max(expl_edges, total_expl_edges - expl_edges);
            if (expl_edges < min_expl_edges) {
                min_expl_edges = expl_edges;
                best_link_to_remove = link;
            }
        }
    }
    return pair<Link, int>(best_link_to_remove, min_expl_edges);
}

int GetExplanationEdgesAgg(LogData *data, 
                           vector<Flow *> *dropped_flows,
                           int ntraces,
                           set<int> &equivalent_devices,
                           set<Link> &removed_links,
                           double max_finish_time_ms){
    map<int, set<Flow *>> flows_by_device;
    for (int ii=0; ii < ntraces; ii++){
        Hypothesis removed_links_hyp;
        for (Link l: removed_links){
            if (data[ii].links_to_ids.find(l) != data[ii].links_to_ids.end())
                removed_links_hyp.insert(data[ii].links_to_ids[l]);
        }
        BinFlowsByDevice(data[ii], max_finish_time_ms, dropped_flows[ii], removed_links_hyp,
                         flows_by_device);
    }
    int ntotal = 0;
    for (int device: equivalent_devices){
        ntotal += flows_by_device[device].size();
    }
    cout << "Total explanation edges " << ntotal << endl;
    return ntotal;
}

PII GetBestLinkToRemove(LogData &data, double max_finish_time_ms,
                        Hypothesis &prev_removed_links,
                        vector<Flow *> &dropped_flows) {
    Hypothesis removed_links;
    Hypothesis result;
    int min_expl_edges = int(1e9);
    int best_link_to_remove = -1;
    for (int link_id = 0; link_id < data.inverse_links.size(); link_id++) {
        Link link = data.inverse_links[link_id];
        int rlink_id = data.GetReverseLinkId(link_id);
        if (data.IsNodeSwitch(link.first) and data.IsNodeSwitch(link.second) and
            !data.IsLinkDevice(link_id)) {
            removed_links = prev_removed_links;
            removed_links.insert(link_id);
            removed_links.insert(rlink_id);
            cout << "Removing link " << link << " (" << link_id << ")" << endl;
            CheckShortestPathExists(data, max_finish_time_ms, dropped_flows,
                                    link_id);
            int expl_edges = GetExplanationEdges(
                data, max_finish_time_ms, dropped_flows, removed_links, result);
            if (expl_edges < min_expl_edges) {
                min_expl_edges = expl_edges;
                best_link_to_remove = link_id;
            }
            result.clear();
        }
    }
    return PII(best_link_to_remove, min_expl_edges);
}

void LocalizeScore(LogData &data, double max_finish_time_ms) {
    Hypothesis removed_links;
    vector<Flow *> dropped_flows;
    GetDroppedFlows(data, dropped_flows);
    int old_expl_edges = int(1.0e9), curr_expl_edges = int(1.0e9);
    int max_iter = 20;
    while (1) {
        auto [best_link_to_remove, expl_edges] = GetBestLinkToRemove(
            data, max_finish_time_ms, removed_links, dropped_flows);
        old_expl_edges = curr_expl_edges;
        curr_expl_edges = expl_edges;
        cout << "Best link to remove "
             << data.inverse_links[best_link_to_remove]
             << " nexplanation edges " << expl_edges << endl;
        //<< " removed_links " << data.IdsToLinks(removed_links) << endl;
        if (old_expl_edges - curr_expl_edges < 10 or max_iter == 0)
            break;
        max_iter--;
        removed_links.insert(best_link_to_remove);
        int rlink_id = data.GetReverseLinkId(best_link_to_remove);
        removed_links.insert(rlink_id);
    }
}

map<PII, pair<int, double>> ReadFailuresBlackHole(string fail_file) {
    auto start_time = chrono::high_resolution_clock::now();
    ifstream infile(fail_file);
    string line;
    char op[30];
    // map[(src, dst)] = (failed_component(link/device), failparam)
    map<PII, pair<int, double>> fails;
    while (getline(infile, line)) {
        char *linec = const_cast<char *>(line.c_str());
        GetString(linec, op);
        if (StringStartsWith(op, "Failing_component")) {
            int src, dest, device; // node1, node2;
            double failparam;
            GetFirstInt(linec, src);
            GetFirstInt(linec, dest);
            // GetFirstInt(linec, node1);
            // GetFirstInt(linec, node2);
            GetFirstInt(linec, device);
            GetFirstDouble(linec, failparam);
            // fails[PII(src, dest)] = pair<Link, double>(Link(node1, node2),
            // failparam);
            fails[PII(src, dest)] = pair<int, double>(device, failparam);
            if (VERBOSE) {
                // cout<< "Failed component "<<src<<" "<<dest<<"
                // ("<<node1<<","<<node2<<") "<<failparam<<endl;
                cout << "Failed component " << src << " " << dest << " "
                     << device << " " << failparam << endl;
            }
        }
    }
    if (VERBOSE) {
        cout << "Read fail file in " << GetTimeSinceSeconds(start_time)
             << " seconds, numfails " << fails.size() << endl;
    }
    return fails;
}

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
