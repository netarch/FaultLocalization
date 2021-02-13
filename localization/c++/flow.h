#ifndef __FAULT_LOCALIZE_FLOW__
#define __FAULT_LOCALIZE_FLOW__

#include <iostream>
#include <vector>
#include <unordered_map>
#include <list>
#include "defs.h"

using namespace std;

class LogData;

struct FlowSnapshot{
    double snapshot_time_ms;
    int packets_sent;
    int packets_lost;
    int packets_randomly_lost;
    FlowSnapshot(double snapshot_time_ms_, int packets_sent_, int packets_lost_,
                 int packets_randomly_lost_):
        snapshot_time_ms(snapshot_time_ms_),
        packets_sent(packets_sent_),
        packets_lost(packets_lost_),
        packets_randomly_lost(packets_randomly_lost_){}
};

class Flow{
public:
    Flow() {}

    // Initialize flow without snapshot
    Flow(int src_, int srcport_, int dest_, int destport_, int nbytes_, double start_time_ms_);

    //!TODO
    Flow(Flow &flow, unordered_map<Link, Link> &reduced_graph_map, LogData &data,
         LogData &reduced_data);

    void AddPath(Path *path, bool is_path_taken=false);
    // A reverse path is from the destination to the source
    void AddReversePath(Path *path, bool is_reverse_path_taken=false);
    void SetPathTaken(Path *path);
    void SetReversePathTaken(Path *path);
    void DoneAddingPaths();

    int GetLatestPacketsSent();
    int GetLatestPacketsLost();

    /* A flow has several snapshots over time.
     * It's assumed that the snapshots will be accessed in non-decreasing order of time
     */
    bool AnySnapshotBefore(double finish_time_ms);
    void AddSnapshot(double snapshot_time_ms, int packets_sent, int packets_lost,
                     int packets_randomly_lost);
    void PrintFlowSnapshots(ostream& out=std::cout);
    void PrintFlowMetrics(ostream& out=std::cout);
    void PrintInfo(ostream& out=std::cout);

    // !Note: Any function which can move across snapshots is not thread-safe.
    //        That includes all functions which take max_finish_time_ms as input

    // !TODO: convert to vector<Path> for better cache performance
    vector<Path*>* GetPaths(double max_finish_time_ms);
    vector<Path*>* GetReversePaths(double max_finish_time_ms);
    Path* GetPathTaken();
    Path* GetReversePathTaken();

    // Forward the snapshot ptr to the latest snapshot before max_finish_time_ms
    inline void UpdateSnapshotPtr(double max_finish_time_ms);
    // According to the latest snapshot before max_finish_time_ms
    double GetDropRate(double max_finish_time_ms);
    int GetPacketsLost(double max_finish_time_ms);
    int GetPacketsSent(double max_finish_time_ms);

    bool IsFlowActive();
    bool TracerouteFlow(double max_finish_time_ms);
    bool IsFlowBad(double max_finish_time_ms);
    bool DiscardFlow();

    // Assign two weights to each flow : (good_weight, bad_weight)
    PII LabelWeightsFunc(double max_finish_time_ms);

    void SetFirstLinkId(int link_id);
    void SetLastLinkId(int link_id);
    void SetReverseFirstLinkId(int link_id);
    void SetReverseLastLinkId(int link_id);

    vector<Path*>* paths, *reverse_paths;
    double start_time_ms;
    int src, dest;
    // Server to ToR links that are common to all paths
    int first_link_id, last_link_id;
    // Server to ToR links that are common to all reverse paths
    int reverse_first_link_id, reverse_last_link_id;
    // Should be of size 1 always
    vector<Path*> path_taken_vector, reverse_path_taken_vector;

    /* For reduced computations */
    int npaths_unreduced, npaths_reverse_unreduced;

    void ResetSnapshotCounter();

    /* To accelerate bayesian net computations, cache expensive intermediate calculation */
    long double cached_intermediate_value;
    void SetCachedIntermediateValue(long double value);
    long double GetCachedIntermediateValue();

    int nbytes;
    vector<FlowSnapshot*> snapshots;
    ~Flow();
private:
    int srcport, destport;
    int curr_snapshot_ptr;
};

#endif
