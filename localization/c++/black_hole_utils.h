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

int GetExplanationEdges(LogData &data, double max_finish_time_ms,
                        vector<Flow *> &dropped_flows,
                        Hypothesis &removed_links, Hypothesis &result);

int GetExplanationEdgesFromMap(map<int, set<Flow *>> &flows_by_device);

PII GetBestLinkToRemove(LogData &data, double max_finish_time_ms,
                        Hypothesis &prev_removed_links,
                        vector<Flow *> &dropped_flows);

void LocalizeScore(LogData &data, double max_finish_time_ms);

map<PII, pair<int, double>> ReadFailuresBlackHole(string fail_file);

void LocalizeProbAnalysis(LogData &data,
                          map<PII, pair<int, double>> &failed_components,
                          double min_start_time_ms, double max_finish_time_ms,
                          int nopenmp_threads);

set<int> GetEquivalentDevices(map<int, set<Flow*>> &flows_by_device);

void LocalizeScoreAgg(vector<pair<string, string>> &in_topo_traces,
                  double max_finish_time_ms, int nopenmp_threads);

pair<Link, int> GetBestLinkToRemoveAgg(LogData *data, vector<Flow*> *dropped_flows, int ntraces, set<int> &equivalent_devices,
                                       set<Link> &prev_removed_links,
                                       double max_finish_time_ms, int nopenmp_threads);
map<int, set<Flow *>> BinFlowsByDeviceAgg(LogData *data,
                         vector<Flow*> *dropped_flows,
                         int ntraces,
                         set<Link> removed_links,
                         double max_finish_time_ms, int nopenmp_threads);

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
#endif
