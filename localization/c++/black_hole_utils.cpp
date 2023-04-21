#include "black_hole_utils.h"
#include "bayesian_net.h"
#include "bayesian_net_continuous.h"
#include "bayesian_net_sherlock.h"
#include "doubleO7.h"
#include "net_bouncer.h"
#include "utils.h"
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

int GetExplanationEdgesFromMap(map<int, set<Flow *>> &flows_by_device) {
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

set<int> GetEquivalentDevices(map<int, set<Flow *>> &flows_by_device) {
    set<int> ret;
    vector<pair<int, set<Flow *>>> sorted_map(flows_by_device.begin(),
                                              flows_by_device.end());
    sort(sorted_map.begin(), sorted_map.end(), SortByValueSize);
    int maxedges = sorted_map[0].second.size();
    for (auto &[device, flows] : sorted_map) {
        if (flows.size() < maxedges)
            break;
        cout << "Eq device " << device << " explains " << flows.size()
             << " drops" << endl;
        ret.insert(device);
    }
    return ret;
}

void LocalizeScoreCA(vector<pair<string, string>> &in_topo_traces,
                     double max_finish_time_ms, int nopenmp_threads) {
    int ntraces = in_topo_traces.size();
    vector<Flow *> dropped_flows[ntraces];
    LogData data[ntraces];
    for (int ii = 0; ii < ntraces; ii++) {
        string topo_f = in_topo_traces[ii].first;
        string trace_f = in_topo_traces[ii].second;
        cout << "calling GetDataFromLogFileParallel on " << topo_f << ", "
             << trace_f << endl;
        GetDataFromLogFileParallel(trace_f, topo_f, &data[ii], nopenmp_threads);
        GetDroppedFlows(data[ii], dropped_flows[ii]);
    }

    set<Link> removed_links;
    map<int, set<Flow *>> flows_by_device_agg;
    BinFlowsByDeviceAgg(data, dropped_flows, ntraces, removed_links,
                        flows_by_device_agg, max_finish_time_ms);
    set<int> equivalent_devices = GetEquivalentDevices(flows_by_device_agg);
    cout << "equivalent devices " << equivalent_devices << " size "
         << equivalent_devices.size() << endl;

    GetExplanationEdgesFromMap(flows_by_device_agg);
    cout << "Explaining drops" << endl;

    int max_iter = 10;
    double last_information = 0.0;
    set<set<int>> eq_device_sets ({equivalent_devices});
    while (1) {
        auto [best_link_to_remove, information] = GetBestLinkToRemoveCA(
            data, dropped_flows, ntraces, equivalent_devices, eq_device_sets,
            max_finish_time_ms, nopenmp_threads);
        cout << "Best link to remove " << best_link_to_remove
             << " information " << information << " removed_links "
             << removed_links << endl;
        if (information - last_information < 1.0e-3 or max_iter == 0)
            break;
        last_information = information;
        max_iter--;
        removed_links.insert(best_link_to_remove);
        Link rlink(best_link_to_remove.second, best_link_to_remove.first);
        removed_links.insert(rlink);
    }
}
void LocalizeScoreAgg(vector<pair<string, string>> &in_topo_traces,
                      double max_finish_time_ms, int nopenmp_threads) {
    int ntraces = in_topo_traces.size();
    vector<Flow *> dropped_flows[ntraces];
    LogData data[ntraces];
    for (int ii = 0; ii < ntraces; ii++) {
        string topo_f = in_topo_traces[ii].first;
        string trace_f = in_topo_traces[ii].second;
        cout << "calling GetDataFromLogFileParallel on " << topo_f << ", "
             << trace_f << endl;
        GetDataFromLogFileParallel(trace_f, topo_f, &data[ii], nopenmp_threads);
        GetDroppedFlows(data[ii], dropped_flows[ii]);
    }

    set<Link> removed_links;
    map<int, set<Flow *>> flows_by_device_agg;
    BinFlowsByDeviceAgg(data, dropped_flows, ntraces, removed_links,
                        flows_by_device_agg, max_finish_time_ms);
    set<int> equivalent_devices = GetEquivalentDevices(flows_by_device_agg);
    cout << "equivalent devices " << equivalent_devices << " size "
         << equivalent_devices.size() << endl;

    GetExplanationEdgesFromMap(flows_by_device_agg);
    cout << "Explaining drops" << endl;

    int old_expl_edges = int(1.0e9), curr_expl_edges = int(1.0e9);
    int max_iter = 10;
    while (1) {
        auto [best_link_to_remove, expl_edges] = GetBestLinkToRemoveAgg(
            data, dropped_flows, ntraces, equivalent_devices, removed_links,
            max_finish_time_ms, nopenmp_threads);
        old_expl_edges = curr_expl_edges;
        curr_expl_edges = expl_edges;
        cout << "Best link to remove " << best_link_to_remove
             << " nexplanation edges " << expl_edges << " removed_links "
             << removed_links << endl;
        // if (old_expl_edges - curr_expl_edges < 10 or max_iter == 0) break;
        if (old_expl_edges - curr_expl_edges < 1 or max_iter == 0)
            break;
        max_iter--;
        removed_links.insert(best_link_to_remove);
        Link rlink(best_link_to_remove.second, best_link_to_remove.first);
        removed_links.insert(rlink);
    }
}

Link GetMostUsedLink(LogData *data, vector<Flow *> *dropped_flows, int ntraces,
                     int device, double max_finish_time_ms,
                     int nopenmp_threads) {
    map<Link, int> link_ctrs;
    for (int tt = 0; tt < ntraces; tt++) {
        // cout << "Done with trace " << tt << endl;
        for (Flow *flow : dropped_flows[tt]) {
            // cout << "Done with flow " << flow << endl;
            vector<Path *> *flow_paths = flow->GetPaths(max_finish_time_ms);
            for (Path *link_path : *flow_paths) {
                Path device_path;
                data[tt].GetDeviceLevelPath(flow, *link_path, device_path);
                for (int ii = 1; ii < device_path.size(); ii++) {
                    if ((device_path[ii - 1] == device or
                         device_path[ii] == device) and
                        (device_path[ii - 1] != device_path[ii])) {
                        Link l(device_path[ii - 1], device_path[ii]);
                        link_ctrs[l]++;
                    }
                }
            }
        }
    }
    map<Link, int>::iterator most_used =
        max_element(link_ctrs.begin(), link_ctrs.end(),
                    [](const std::pair<Link, int> &a,
                       const std::pair<Link, int> &b) -> bool {
                        return a.second < b.second;
                    });
    return most_used->first;
}

pair<Link, int>
GetBestLinkToRemoveAgg(LogData *data, vector<Flow *> *dropped_flows,
                       int ntraces, set<int> &equivalent_devices,
                       set<Link> &prev_removed_links, double max_finish_time_ms,
                       int nopenmp_threads) {
    set<Link> removed_links;
    int total_expl_edges =
        GetExplanationEdgesAgg(data, dropped_flows, ntraces, equivalent_devices,
                               removed_links, max_finish_time_ms);
    int min_expl_edges = int(1e9);
    Link best_link_to_remove = Link(-1, -1);
    for (Link link : data[0].inverse_links) {
        int link_id = data[0].links_to_ids[link];
        Link rlink = Link(link.second, link.first);
        if (data[0].IsNodeSwitch(link.first) and
            data[0].IsNodeSwitch(link.second) and
            !data[0].IsLinkDevice(link_id)) {
            removed_links = prev_removed_links;
            removed_links.insert(link);
            removed_links.insert(rlink);
            cout << "Removing link " << link << endl;
            for (int ii = 0; ii < ntraces; ii++) {
                int link_id_ii = data[ii].links_to_ids[link];
                CheckShortestPathExists(data[ii], max_finish_time_ms,
                                        dropped_flows[ii], link_id_ii);
            }

            int expl_edges = GetExplanationEdgesAgg(
                data, dropped_flows, ntraces, equivalent_devices, removed_links,
                max_finish_time_ms);
            // expl_edges = max(expl_edges, total_expl_edges - expl_edges);
            if (expl_edges < min_expl_edges or
                (expl_edges == min_expl_edges and
                 (equivalent_devices.find(link.first) !=
                      equivalent_devices.end() or
                  equivalent_devices.find(link.second) !=
                      equivalent_devices.end()))) {
                min_expl_edges = expl_edges;
                best_link_to_remove = link;
            }
        }
    }
    return pair<Link, int>(best_link_to_remove, min_expl_edges);
}

void BinFlowsByDeviceAgg(LogData *data, vector<Flow *> *dropped_flows,
                         int ntraces, set<Link> &removed_links,
                         map<int, set<Flow *>> &flows_by_device,
                         double max_finish_time_ms) {
    for (int ii = 0; ii < ntraces; ii++) {
        Hypothesis removed_links_hyp;
        for (Link l : removed_links) {
            if (data[ii].links_to_ids.find(l) != data[ii].links_to_ids.end())
                removed_links_hyp.insert(data[ii].links_to_ids[l]);
        }
        BinFlowsByDevice(data[ii], max_finish_time_ms, dropped_flows[ii],
                         removed_links_hyp, flows_by_device);
    }
}

void GetEqDevicesInFlowPaths(LogData &data, Flow *flow,
                             set<int> &equivalent_devices,
                             Hypothesis &removed_links,
                             double max_finish_time_ms, set<int> &result) {
    vector<Path *> *flow_paths = flow->GetPaths(max_finish_time_ms);
    for (Path *link_path : *flow_paths) {
        if (!HypothesisIntersectsPath(&removed_links, link_path)) {
            Path device_path;
            data.GetDeviceLevelPath(flow, *link_path, device_path);
            for (int device : device_path) {
                if (equivalent_devices.find(device) !=
                    equivalent_devices.end()) {
                    result.insert(device);
                }
            }
        }
    }
}

// CA: coloring algorithm
void GetEqDeviceSetsCA(LogData *data, vector<Flow *> *dropped_flows,
                       int ntraces, set<int> &equivalent_devices,
                       Link removed_link, double max_finish_time_ms,
                       set<set<int>> &result) {
    int link_id = data[0].links_to_ids[removed_link];
    int rlink_id = data[0].GetReverseLinkId(link_id);
    Hypothesis removed_links({link_id, rlink_id});

    // do it for flows from the first trace in the unchanged network
    for (Flow *flow : dropped_flows[0]) {
        set<int> flow_eq_devices;
        GetEqDevicesInFlowPaths(data[0], flow, equivalent_devices,
                                removed_links, max_finish_time_ms,
                                flow_eq_devices);
        if (result.find(flow_eq_devices) == result.end()) {
            result.insert(flow_eq_devices);
        }
    }
}

// CA: coloring algorithm
// Replacement of GetExplanationEdgesAgg
// eq_device_sets should at least have the entire equivalent_devices set
double GetEqDeviceSetsMeasureCA(LogData *data, vector<Flow *> *dropped_flows,
                                int ntraces, set<int> &equivalent_devices,
                                Link removed_link, double max_finish_time_ms,
                                set<set<int>> &eq_device_sets) {
    GetEqDeviceSetsCA(data, dropped_flows, ntraces, equivalent_devices,
                      removed_link, max_finish_time_ms, eq_device_sets);

    /*
        Use a coloring algorithm to find all possible intersection sets
    */

    // initialize colors
    int curr_color = 0;
    map<int, int> device_colors;
    for (int d : equivalent_devices)
        device_colors[d] = curr_color;

    for (auto &eq_device_set : eq_device_sets) {
        // update colors of all devices in eq_device_set
        // mapping from old color to new color
        map<int, int> col_newcol;
        for (int d : eq_device_set) {
            // d should be a device in equivalent_devices
            assert(device_colors.find(d) != device_colors.end());
            int c = device_colors[d];
            if (col_newcol.find(c) == col_newcol.end()) {
                col_newcol[c] = curr_color++;
            }
            device_colors[d] = col_newcol[c];
        }
    }

    map<int, int> col_cnts;
    for (auto [d, c] : device_colors)
        col_cnts[c]++;

    double information = 0.0;
    for (auto [c, cnt] : col_cnts) {
        double p = cnt / equivalent_devices.size();
        information += -p * log(p);
    }
    return information;
}

// CA: coloring algorithm
pair<Link, double> GetBestLinkToRemoveCA(LogData *data,
                                         vector<Flow *> *dropped_flows,
                                         int ntraces,
                                         set<int> &equivalent_devices,
                                         set<set<int>> &eq_device_sets,
                                         double max_finish_time_ms,
                                         int nopenmp_threads) {
    double max_information = 0.0;
    Link best_link_to_remove = Link(-1, -1);
    for (Link link : data[0].inverse_links) {
        int link_id = data[0].links_to_ids[link];
        Link rlink = Link(link.second, link.first);
        if (data[0].IsNodeSwitch(link.first) and
            data[0].IsNodeSwitch(link.second) and
            !data[0].IsLinkDevice(link_id)) {
            cout << "Removing link " << link << endl;
            for (int ii = 0; ii < ntraces; ii++) {
                int link_id_ii = data[ii].links_to_ids[link];
                CheckShortestPathExists(data[ii], max_finish_time_ms,
                                        dropped_flows[ii], link_id_ii);
            }
            set<set<int>> &eq_device_sets_copy = eq_device_sets;
            double information = GetEqDeviceSetsMeasureCA(
                data, dropped_flows, ntraces, equivalent_devices, link,
                max_finish_time_ms, eq_device_sets_copy);
            if (information > max_information) {
                best_link_to_remove = link;
                max_information = information;
            }
        }
    }
    // Do again for the best link to populate eq_device_sets
    GetEqDeviceSetsMeasureCA(data, dropped_flows, ntraces, equivalent_devices,
                             best_link_to_remove, max_finish_time_ms,
                             eq_device_sets);
    return pair<Link, double>(best_link_to_remove, max_information);
}

int GetExplanationEdgesAgg2(LogData *data, vector<Flow *> *dropped_flows,
                            int ntraces, set<int> &equivalent_devices,
                            set<Link> &removed_links,
                            double max_finish_time_ms) {
    map<int, set<Flow *>> flows_by_device;
    BinFlowsByDeviceAgg(data, dropped_flows, ntraces, removed_links,
                        flows_by_device, max_finish_time_ms);
    set<int> new_eq_devices = GetEquivalentDevices(flows_by_device);
    int ret =
        new_eq_devices.size(); // * 10000000 + 100 - (maxedges - nextedges);
    cout << "Total explanation edges " << ret << endl;
    return ret;
}

int GetExplanationEdgesAgg(LogData *data, vector<Flow *> *dropped_flows,
                           int ntraces, set<int> &equivalent_devices,
                           set<Link> &removed_links,
                           double max_finish_time_ms) {
    map<int, set<Flow *>> flows_by_device;
    BinFlowsByDeviceAgg(data, dropped_flows, ntraces, removed_links,
                        flows_by_device, max_finish_time_ms);
    int ntotal = 0;
    for (int device : equivalent_devices) {
        cout << "Flows for device " << device << " "
             << flows_by_device[device].size() << endl;
        ntotal += flows_by_device[device].size();
    }
    cout << "Total explanation edges " << ntotal << endl;
    return ntotal;
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
