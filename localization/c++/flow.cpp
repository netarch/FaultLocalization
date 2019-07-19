#include "flow.h"
#include <assert.h>

void Flow::AddPath(Path *path, bool is_path_taken){
    paths.push_back(path);
    if (is_path_taken) {
        SetPathTaken(path);
    }
}

void Flow::AddReversePath(Path *path, bool is_reverse_path_taken){
    reverse_paths.push_back(path);
    if (is_reverse_path_taken) {
        SetReversePathTaken(path);
    }
}

void Flow::SetPathTaken(Path *path){
    assert (path_taken_vector.size() == 0);
    path_taken_vector.push_back(path);
}

void Flow::SetReversePathTaken(Path *path){
    assert (reverse_path_taken_vector.size() == 0);
    reverse_path_taken_vector.push_back(path);
}

int Flow::GetLatestPacketsSent(){
    if (snapshots.empty()) return 0;
    return snapshots.back()->packets_sent;
}

int Flow::GetLatestPacketsLost(){
    if (snapshots.empty()) return 0;
    return snapshots.back()->packets_lost;
}

bool Flow::AnySnapshotBefore(double finish_time_ms){
    return (snapshots.size()>0 && snapshots[0]->snapshot_time_ms <= finish_time_ms);
}

void Flow::AddSnapshot(double snapshot_time_ms, int packets_sent, int packets_lost, int packets_randomly_lost){
    FlowSnapshot* snapshot = new FlowSnapshot(snapshot_time_ms, packets_sent, packets_lost, packets_randomly_lost);
    snapshots.push_back(snapshot);
}

void Flow::PrintFlowSnapshots(ostream& out){
    for (FlowSnapshot* snapshot: snapshots){
        out<<"Snapshot "<<snapshot->snapshot_time_ms<<" "
           <<snapshot->packets_sent<<" "<<snapshot->packets_lost<<" "
           <<snapshot->packets_randomly_lost<<endl;
    }
}

void Flow::PrintFlowMetrics(ostream& out){
    out<<"Flowid= "<<src<<" "<<dest<<" "<<nbytes<<" "<<start_time_ms<<endl;
}

void Flow::PrintInfo(ostream& out){
    PrintFlowMetrics(out);
    PrintFlowSnapshots(out);
    for (Path* path: paths){
        out<<"flowpath";
        if (path == path_taken_vector[0]){
            out<<"_taken";
        }
        for (int n: *path){
            out<<" "<<n;
        }
        out<<endl;
    }
    for (Path* path: reverse_paths){
        out<<"flowpath_reverse";
        if (path == reverse_path_taken_vector[0]){
            out<<"_taken";
        }
        for (int n: *path){
            out<<" "<<n;
        }
        out<<endl;
    }
}

vector<Path*>* Flow::GetPaths(double max_finish_time_ms){
    if (PATH_KNOWN or TracerouteFlow(max_finish_time_ms)){
        assert (path_taken_vector.size() == 1);
        return &path_taken_vector;
    }
    return &paths;
}

vector<Path*>* Flow::GetReversePaths(double max_finish_time_ms){
    if (PATH_KNOWN or TracerouteFlow(max_finish_time_ms)){
        assert (reverse_path_taken_vector.size() == 1);
        return &reverse_path_taken_vector;
    }
    return &reverse_paths;
}

void Flow::UpdateSnapshotPtr(double max_finish_time_ms){
    assert (curr_snapshot_ptr == -1 ||
            snapshots[curr_snapshot_ptr]->snapshot_time_ms < max_finish_time_ms);
    while (curr_snapshot_ptr + 1 < snapshots.size() && 
           snapshots[curr_snapshot_ptr+1]->snapshot_time_ms < max_finish_time_ms){
        curr_snapshot_ptr += 1;
    }
}

double Flow::GetDropRate(double max_finish_time_ms){
    UpdateSnapshotPtr(max_finish_time_ms);
    if (curr_snapshot_ptr >= 0){
        return ((float)snapshots[curr_snapshot_ptr]->packets_lost)/snapshots[curr_snapshot_ptr]->packets_sent;
    }
    return 0.0;
}

int Flow::GetPacketsLost(double max_finish_time_ms){
    UpdateSnapshotPtr(max_finish_time_ms);
    if (curr_snapshot_ptr >= 0){
        return snapshots[curr_snapshot_ptr]->packets_lost;
    }
    return 0;
}

bool Flow::IsFlowActive(){
    return (paths.size() == 1);
}

bool Flow::TracerouteFlow(double max_finish_time_ms){
    return (PATH_KNOWN || GetPacketsLost(max_finish_time_ms) > 0 || IsFlowActive());
}

bool Flow::IsFlowBad(double max_finish_time_ms){
    return (GetPacketsLost(max_finish_time_ms) > 0);
}

PII Flow::LabelWeightsFunc(double max_finish_time_ms){
    UpdateSnapshotPtr(max_finish_time_ms);
    int gw = 0, bw = 0;
    if (curr_snapshot_ptr >= 0){
        gw = snapshots[curr_snapshot_ptr]->packets_sent - snapshots[curr_snapshot_ptr]->packets_lost;
        bw = snapshots[curr_snapshot_ptr]->packets_lost;
    }
    return PII(gw, bw);
}

bool Flow::DiscardFlow(){
    return false;
    //return !(src < 256 && dest <256)
}
