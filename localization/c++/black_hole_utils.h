#ifndef __BLACK_HOLE_UTILS__
#define __BLACK_HOLE_UTILS__

#include "utils.h"
#include <bits/stdc++.h>
#include <chrono>
#include <iostream>

inline bool SortByValueSize(const pair<int, set<Flow *>> &a,
                     const pair<int, set<Flow *>> &b) {
    return (a.second.size() > b.second.size());
}

// Check if removing a link does not eliminate all shortest paths
void CheckShortestPathExists(LogData &data, double max_finish_time_ms,
                             vector<Flow *> &dropped_flows,
                             int link_to_remove);

// ... in the modified network
void BinFlowsByDevice(LogData &data, double max_finish_time_ms,
                      vector<Flow *> &dropped_flows, Hypothesis &removed_links,
                      map<int, set<Flow *>> &flows_by_device);

int GetExplanationEdgesFromMap(map<int, set<Flow *>> &flows_by_device);

map<PII, pair<int, double>> ReadFailuresBlackHole(string fail_file);

set<int> GetEquivalentDevices(map<int, set<Flow*>> &flows_by_device);

void LocalizeScoreAgg(vector<pair<string, string>> &in_topo_traces,
                  double max_finish_time_ms, int nopenmp_threads);

pair<Link, int> GetBestLinkToRemoveAgg(LogData *data, vector<Flow*> *dropped_flows, int ntraces, set<int> &equivalent_devices,
                                       set<Link> &prev_removed_links,
                                       double max_finish_time_ms, int nopenmp_threads);

void BinFlowsByDeviceAgg(LogData *data,
                         vector<Flow*> *dropped_flows,
                         int ntraces,
                         set<Link> &removed_links,
                         map<int, set<Flow *>> &flows_by_device,
                         double max_finish_time_ms);

int GetExplanationEdgesAgg(LogData *data, 
                           vector<Flow *> *dropped_flows,
                           int ntraces,
                           set<int> &equivalent_devices,
                           set<Link> &removed_links,
                           double max_finish_time_ms);

int GetExplanationEdgesAgg2(LogData *data, 
                           vector<Flow *> *dropped_flows,
                           int ntraces,
                           set<int> &equivalent_devices,
                           set<Link> &removed_links,
                           double max_finish_time_ms);

Link GetMostUsedLink(LogData *data, vector<Flow *> *dropped_flows,
                       int ntraces, int device, double max_finish_time_ms,
                       int nopenmp_threads);

/* 
  Coloring based scheme
  Uses information theoretic measure of sets to identify best link removal sequence
*/
void LocalizeScoreCA(vector<pair<string, string>> &in_topo_traces,
                     double max_finish_time_ms, int nopenmp_threads);

void GetEqDeviceSetsCA(LogData *data, vector<Flow *> *dropped_flows,
                       int ntraces, set<int> &equivalent_devices,
                       Link removed_link, double max_finish_time_ms,
                       set<set<int>> &result);

double GetEqDeviceSetsMeasureCA(LogData *data, vector<Flow *> *dropped_flows,
                                int ntraces, set<int> &equivalent_devices,
                                Link removed_link, double max_finish_time_ms,
                                set<set<int>> &eq_device_sets);

pair<Link, double> GetBestLinkToRemoveCA(LogData *data,
                                         vector<Flow *> *dropped_flows,
                                         int ntraces,
                                         set<int> &equivalent_devices,
                                         set<set<int>> &eq_device_sets,
                                         double max_finish_time_ms,
                                         int nopenmp_threads);

void GetEqDevicesInFlowPaths(LogData &data, Flow *flow,
                             set<int> &equivalent_devices,
                             Hypothesis &removed_links,
                             double max_finish_time_ms, set<int> &result);
#endif
