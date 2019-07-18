#include "flow.h"

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

void Flow::SetPathTaken(path *path){
    assert (path_taken_vector.size() == 0);
    path_taken_vector.push_back(path);
}

void Flow::SetReversePathTaken(path *path){
    assert (reverse_path_taken_vector.size() == 0);
    reverse_path_taken_vector.push_back(path);
}

int Flow::GetLatestPacketsSent(){
    if (snapshots.empty()) return 0;
    return snapshots.back()->packets_sent;
}

int Flow::GetLatestPacketsLost();
    if (snapshots.empty()) return 0;
    return snapshots.back()->packets_lost;
}

bool Flow::AnySnapshotBefore(double finish_time_ms){
    return (snapshots.size()>0 && snapshots[0]->snapshot_time_ms <= finish_time_ms);
}

void Flow::AddSnapshot(double snapshot_time_ms, int packets_sent, int packets_lost, int packets_randomly_lost){
    FlowSnapshot* snapshot = FlowSnapshot(snapshot_time_ms, packets_sent, packets_lost, packets_randomly_lost);
    snapshots.push_back(snapshot);
}

void Flow::PrintFlowSnapshots(ostream& out){
    for (FlowSnapshot* snapshot: snapshots){
        out<<"Snapshot "<<snapshot_time_ms<<" "
           <<packets_sent<<" "<<lost_packets<<" "
           <<randomly_lost_packets<<endl;
    }
}

void Flow::PrintFlowMetrics(ostream& out){
    out<<"Flowid= "<<src<<" "<<dest<<" "<<nbytes<<" "<<start_time_ms<<endl;
}

void Flow::PrintInfo(ostream& out){
    PrintFlowMetrics(out);
    PrintFlowSnapshots(out);
    for (Path* path: paths){
        out<<"flowpath"
        if (path == path_taken_vector[0]){
            out<<"_taken"
        }
        for (int n: *path){
            out<<" "<<n;
        }
        out<<endl;
    }
    for (Path* path: reverse_paths){
        out<<"flowpath_reverse"
        if (path == reverse_path_taken_vector[0]){
            out<<"_taken"
        }
        for (int n: *path){
            out<<" "<<n;
        }
        out<<endl;
    }
}

vector<Path*>* Flow::GetPaths(double max_finish_time_ms){
    if (PATH_KNOWN or TracerouteFlow(max_finish_time_ms)){
        return &path_taken_vector;
    }
    return &paths;
}

vector<Path*>* Flow::GetReversePaths(double max_finish_time_ms){
    if (PATH_KNOWN or TracerouteFlow(max_finish_time_ms)){
        return &reverse_path_taken_vector;
    }
    return &reverse_paths;
}

void Flow::UpdateSnapshotPtr(double max_finish_time_ms){
    assert (curr_snapshot_ptr == -1 ||
            snapshots[ptr].snapshot_time_ms < max_finish_time_ms);
    while (curr_snapshot_ptr + 1 < snapshots.size() && 
           snapshots[curr_snapshot_ptr+1].snapshot_time_ms < max_finish_time_ms){
        curr_snapshot_ptr += 1;
    }
}
