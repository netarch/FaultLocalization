#include <iostream>
#include <vector>
#include <list>

#include "defs.h"

using namespace std;


struct FlowSnapshot{
    double snapshot_time_ms;
    int packets_sent;
    int pacekts_lost;
    int packets_randomly_lost;
    FlowSnapshot(double snapshot_time_ms_, int packets_sent_, int packets_lost_, int packets_randomly_lost_):
        snapshot_time_ms(snapshot_time_ms_),
        packets_sent(packets_sent_),
        packets_lost(packets_lost_),
        packets_randomly_lost(packets_randomly_lost_){}
};

class Flow{
public:
    // Initialize flow without snapshot
    Flow(int src, string srcip, int srcport, int dest, string destip, int destport, int nbytes, double start_time_ms);

    void AddPath(Path *path, bool path_taken=false):
    // A reverse path is from the destination to the source
    void AddReversePath(Path *path, bool reverse_path_taken=false):
    void SetPathTaken(path *path):
    void SetReversePathTaken(path *path):

    int GetLatestPacketsSent();
    int GetLatestPacketsLost();

    /* A flow has several snapshots over time.
     * It's assumed that the snapshots will be accessed in non-decreasing order of time
     */
    bool AnySnapshotBefore(double finish_time_ms);
    void AddSnapshot(double snapshot_time_ms, int packets_sent, int lost_packets, int randomly_lost_packets);
    void PrintFlowSnapshots(ostream& out=std::cout);
    void PrintFlowMetrics(ostream& out=std::cout);
    void PrintInfo(ostream& out=std::cout);

    vector<Path*>* GetPaths(double max_finish_time_ms);
    vector<Path*>* GetReversePaths(double max_finish_time_ms);
    bool FlowFinished();

    // Forward the snapshot ptr to the latest snapshot before max_finish_time_ms
    void UpdateSnapshotPtr(double max_finish_time_ms);
    // According to the latest snapshot before max_finish_time_ms
    double GetDropRate(double max_finish_time_ms);
    int GetPacketsLost(double max_finish_time_ms);

    bool IsFlowActive();
    bool TracerouteFlow(double max_finish_time_ms);
    bool IsFlowBad(double max_finish_time_ms);
    bool DiscardFlow();

    // Assign two weights to each flow : (good_weight, bad_weight)
    PII LabelWeightsFunc(double max_finish_time_ms):

private:
    int src, dest;
    string srcip, destip;
    int srcport, destport;
    int nbytes;
    int start_time_ms;
    vector<FlowSnapshot*> snapshots;
    int curr_snapshot_ptr;
    vector<Path*> paths, reverse_paths;
    Path* path_taken, reverse_path_taken;
};
