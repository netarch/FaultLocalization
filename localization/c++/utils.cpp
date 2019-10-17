#include "utils.h"
#include "logdata.h"
#include <assert.h>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <omp.h>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <sparsehash/dense_hash_map>
using google::dense_hash_map;

inline bool StringStartsWith(const string &a, const string &b) {
    return (b.length() <= a.length() && equal(b.begin(), b.end(), a.begin()));
}

void GetDataFromLogFileDistributed(string dirname, int nchunks, LogData *result, int nopenmp_threads){ 
    auto start_time = chrono::high_resolution_clock::now();
    #pragma omp parallel for num_threads(nopenmp_threads)
    for(int i=0; i<nchunks; i++){
        GetDataFromLogFile(dirname + "/" + to_string(i), result);
    }
    if constexpr (VERBOSE){
        cout<<"Read log file chunks in "<<GetTimeSinceMilliSeconds(start_time) << " seconds"<<endl;
    }
}

void GetDataFromLogFile(string filename, LogData *result){ 
    auto start_time = chrono::high_resolution_clock::now();
    vector<Flow*> chunk_flows;
    dense_hash_map<Link, int, hash<Link> > links_to_ids_cache;
    links_to_ids_cache.set_empty_key(Link(-1, -1));
    auto GetLinkId =
        [&links_to_ids_cache, result] (Link link){
            int link_id;
            auto it = links_to_ids_cache.find(link);
            if (it == links_to_ids_cache.end()){
                link_id = result->GetLinkId(link);
                links_to_ids_cache[link] = link_id;
            }
            else{
                link_id = it->second;
            }
            return link_id;
        };
    vector<int> temp_path (10);
    vector<int> path_nodes (10);
    ifstream infile(filename);
    string line, op;
    Flow *flow = NULL;
    int curr_link_index = 0;
    int nlines = 0;
    while (getline(infile, line)){
        char* linec = const_cast<char*>(line.c_str());
        GetString(linec, op);
        //cout << op <<endl;
        //istringstream line_stream (line);
        //sscanf(linum, "%s *", op);
        //line_stream >> op;
        //cout << "op " << op << " : " << line << endl;
        nlines += 1;
        if (StringStartsWith(op, "FPRT")){
            assert (flow != NULL);
            temp_path.clear();
            path_nodes.clear();
            int node;
            // This is only the portion from dest_rack to src_rack
            while (GetFirstInt(linec, node)){
                if (path_nodes.size() > 0){
                    temp_path.push_back(GetLinkId(Link(path_nodes.back(), node)));
                }
                path_nodes.push_back(node);
            }
            assert(path_nodes.size()>0); // No direct link for src_host to dest_host or vice_versa
            MemoizedPaths *memoized_paths = result->GetMemoizedPaths(path_nodes[0], *path_nodes.rbegin());;
            Path* path = memoized_paths->GetPath(temp_path);
            flow->SetReversePathTaken(path);
        }
        else if (StringStartsWith(op, "FPT")){
            assert (flow != NULL);
            temp_path.clear();
            path_nodes.clear();
            int node;
            // This is only the portion from src_rack to dest_rack
            while (GetFirstInt(linec, node)){
                if (path_nodes.size() > 0){
                    temp_path.push_back(GetLinkId(Link(path_nodes.back(), node)));
                }
                path_nodes.push_back(node);
            }
            assert(path_nodes.size()>0); // No direct link for src_host to dest_host or vice_versa
            MemoizedPaths *memoized_paths = result->GetMemoizedPaths(path_nodes[0], *path_nodes.rbegin());
            Path *path = memoized_paths->GetPath(temp_path);
            flow->SetPathTaken(path);
        }
        else if (StringStartsWith(op, "FP")){
            assert (flow == NULL);
            temp_path.clear();
            path_nodes.clear();
            int node;
            // This is only the portion from src_rack to dest_rack
            while (GetFirstInt(linec, node)){
                if (path_nodes.size() > 0){
                    temp_path.push_back(GetLinkId(Link(path_nodes.back(), node)));
                }
                path_nodes.push_back(node);
            }
            assert(path_nodes.size()>0); // No direct link for src_host to dest_host or vice_versa
            MemoizedPaths *memoized_paths = result->GetMemoizedPaths(path_nodes[0], *path_nodes.rbegin());
            memoized_paths->AddPath(temp_path);
            //memoized_paths->GetPath(temp_path);
            //cout << temp_path << endl;
        }
        else if (op == "SS"){
            double snapshot_time_ms;
            int packets_sent, packets_lost, packets_randomly_lost;
            GetFirstDouble(linec, snapshot_time_ms);
            GetFirstInt(linec, packets_sent);
            GetFirstInt(linec, packets_lost);
            GetFirstInt(linec, packets_randomly_lost);
            assert (flow != NULL);
            flow->AddSnapshot(snapshot_time_ms, packets_sent, packets_lost, packets_randomly_lost);
        }
        else if (op == "FID"){
            // Log the previous flow
            if (flow != NULL) flow->DoneAddingPaths();
            if (flow != NULL and flow->paths.size() > 0 and flow->path_taken_vector.size() == 1){
                assert (flow->GetPathTaken());
                if constexpr (CONSIDER_REVERSE_PATH){
                    assert (flow->GetReversePathTaken());
                }
                if (flow->GetLatestPacketsSent() > 0 and !flow->DiscardFlow()
                    and flow->paths.size() > 0){
                    chunk_flows.push_back(flow);
                }
            }
            int src, dest, srcrack, destrack, nbytes;
            double start_time_ms;
            GetFirstInt(linec, src);
            GetFirstInt(linec, dest);
            GetFirstInt(linec, srcrack);
            GetFirstInt(linec, destrack);
            GetFirstInt(linec, nbytes);
            GetFirstDouble(linec, start_time_ms);
            flow = new Flow(src, "", 0, dest, "", 0, nbytes, start_time_ms);
            // Set flow paths
            MemoizedPaths *memoized_paths = result->GetMemoizedPaths(srcrack, destrack);
            memoized_paths->GetAllPaths(flow->paths);
            flow->SetFirstLinkId(GetLinkId(Link(flow->src, srcrack)));
            flow->SetLastLinkId(GetLinkId(Link(destrack, flow->dest)));
        }
        else if (op == "Failing_link"){
            int src, dest;
            double failparam;
            GetFirstInt(linec, src);
            GetFirstInt(linec, dest);
            GetFirstDouble(linec, failparam);
            //sscanf (linum + op.size(),"%d %*d %f", &src, &dest, &failparam);
            result->AddFailedLink(Link(src, dest), failparam);
            if constexpr (VERBOSE){
                cout<< "Failed link "<<src<<" "<<dest<<" "<<failparam<<endl; 
            }
        }
        else if (op == "link_statistics"){
            assert (false);
        }
    }
    // Log the last flow
    if (flow != NULL) flow->DoneAddingPaths();
    if (flow != NULL and flow->paths.size() > 0 and flow->path_taken_vector.size() == 1){
        assert (flow->GetPathTaken());
        if constexpr (CONSIDER_REVERSE_PATH){
            assert (flow->GetReversePathTaken());
        }
        if (flow->GetLatestPacketsSent() > 0 and !flow->DiscardFlow() and flow->paths.size() > 0){
            //flow->PrintInfo();
            chunk_flows.push_back(flow);
        }
    }
    result->AddChunkFlows(chunk_flows);
    if constexpr (VERBOSE){
        cout<< "Read log file in "<<GetTimeSinceMilliSeconds(start_time)
            << " seconds, numlines " << nlines << endl;
    }
}

void GetDataFromLogFile1(string filename, LogData *result){ 
    auto start_time = chrono::high_resolution_clock::now();
    vector<Flow*> chunk_flows;
    dense_hash_map<Link, int, hash<Link> > links_to_ids_cache;
    links_to_ids_cache.set_empty_key(Link(-1, -1));
    auto GetLinkId =
        [&links_to_ids_cache, result] (Link link){
            int link_id;
            auto it = links_to_ids_cache.find(link);
            if (it == links_to_ids_cache.end()){
                link_id = result->GetLinkId(link);
                links_to_ids_cache[link] = link_id;
            }
            else{
                link_id = it->second;
            }
            return link_id;
        };
    vector<int> temp_path (10);
    vector<int> path_nodes (10);
    ifstream infile(filename);
    string line, op;
    Flow *flow = NULL;
    int curr_link_index = 0;
    int nlines = 0;
    while (getline(infile, line)){
        //const char *linum = line.c_str();
        istringstream line_stream (line);
        //cout << linum <<endl;
        //sscanf(linum, "%s *", op);
        line_stream >> op;
        //cout << "op " << op << " : " << line << endl;
        nlines += 1;
        if (StringStartsWith(op, "FPRT")){
            assert (flow != NULL);
            temp_path.clear();
            path_nodes.clear();
            int node;
            // This is only the portion from dest_rack to src_rack
            while (line_stream >> node){
                if (path_nodes.size() > 0){
                    temp_path.push_back(GetLinkId(Link(path_nodes.back(), node)));
                }
                path_nodes.push_back(node);
            }
            assert(path_nodes.size()>0); // No direct link for src_host to dest_host or vice_versa
            MemoizedPaths *memoized_paths = result->GetMemoizedPaths(path_nodes[0], *path_nodes.rbegin());;
            Path* path = memoized_paths->GetPath(temp_path);
            flow->SetReversePathTaken(path);
        }
        else if (StringStartsWith(op, "FPT")){
            assert (flow != NULL);
            temp_path.clear();
            path_nodes.clear();
            int node;
            // This is only the portion from src_rack to dest_rack
            while (line_stream >> node){
                if (path_nodes.size() > 0){
                    temp_path.push_back(GetLinkId(Link(path_nodes.back(), node)));
                }
                path_nodes.push_back(node);
            }
            assert(path_nodes.size()>0); // No direct link for src_host to dest_host or vice_versa
            MemoizedPaths *memoized_paths = result->GetMemoizedPaths(path_nodes[0], *path_nodes.rbegin());
            Path *path = memoized_paths->GetPath(temp_path);
            flow->SetPathTaken(path);
        }
        else if (StringStartsWith(op, "FP")){
            assert (flow == NULL);
            temp_path.clear();
            path_nodes.clear();
            int node;
            // This is only the portion from src_rack to dest_rack
            while (line_stream >> node){
                if (path_nodes.size() > 0){
                    temp_path.push_back(GetLinkId(Link(path_nodes.back(), node)));
                }
                path_nodes.push_back(node);
            }
            assert(path_nodes.size()>0); // No direct link for src_host to dest_host or vice_versa
            MemoizedPaths *memoized_paths = result->GetMemoizedPaths(path_nodes[0], *path_nodes.rbegin());
            //memoized_paths->GetPath(temp_path);
            memoized_paths->AddPath(temp_path);
        }
        else if (op == "SS"){
            double snapshot_time_ms;
            int packets_sent, packets_lost, packets_randomly_lost;
            line_stream >> snapshot_time_ms >> packets_sent >> packets_lost >> packets_randomly_lost;
            assert (flow != NULL);
            flow->AddSnapshot(snapshot_time_ms, packets_sent, packets_lost, packets_randomly_lost);
        }
        else if (op == "FID"){
            // Log the previous flow
            if (flow != NULL) flow->DoneAddingPaths();
            if (flow != NULL and flow->paths.size() > 0 and flow->path_taken_vector.size() == 1){
                assert (flow->GetPathTaken());
                if constexpr (CONSIDER_REVERSE_PATH){
                    assert (flow->GetReversePathTaken());
                }
                if (flow->GetLatestPacketsSent() > 0 and !flow->DiscardFlow()
                    and flow->paths.size() > 0){
                    chunk_flows.push_back(flow);
                }
            }
            int src, dest, srcrack, destrack, nbytes;
            double start_time_ms;
            line_stream >> src >> dest >> srcrack >> destrack >> nbytes >> start_time_ms;
            flow = new Flow(src, "", 0, dest, "", 0, nbytes, start_time_ms);
            // Set flow paths
            MemoizedPaths *memoized_paths = result->GetMemoizedPaths(srcrack, destrack);
            memoized_paths->GetAllPaths(flow->paths);
            flow->SetFirstLinkId(GetLinkId(Link(flow->src, srcrack)));
            flow->SetLastLinkId(GetLinkId(Link(destrack, flow->dest)));
        }
        else if (op == "Failing_link"){
            int src, dest;
            float failparam;
            line_stream >> src >> dest >> failparam;
            //sscanf (linum + op.size(),"%d %*d %f", &src, &dest, &failparam);
            result->AddFailedLink(Link(src, dest), failparam);
            if constexpr (VERBOSE){
                cout<< "Failed link "<<src<<" "<<dest<<" "<<failparam<<endl; 
            }
        }
        else if (op == "link_statistics"){
            assert (false);
        }
    }
    // Log the last flow
    if (flow != NULL) flow->DoneAddingPaths();
    if (flow != NULL and flow->paths.size() > 0 and flow->path_taken_vector.size() == 1){
        assert (flow->GetPathTaken());
        if constexpr (CONSIDER_REVERSE_PATH){
            assert (flow->GetReversePathTaken());
        }
        if (flow->GetLatestPacketsSent() > 0 and !flow->DiscardFlow() and flow->paths.size() > 0){
            //flow->PrintInfo();
            chunk_flows.push_back(flow);
        }
    }
    result->AddChunkFlows(chunk_flows);
    if constexpr (VERBOSE){
        cout<< "Read log file in "<<GetTimeSinceMilliSeconds(start_time)
            << " seconds, numlines " << nlines << endl;
    }
}

PDD GetPrecisionRecall(Hypothesis& failed_links, Hypothesis& predicted_hypothesis){
    vector<int> correctly_predicted;
    for (int link_id: predicted_hypothesis){
        if (failed_links.find(link_id) != failed_links.end()){
            correctly_predicted.push_back(link_id);
        }
    }
    double precision = 1.0, recall = 1.0;
    if (predicted_hypothesis.size() > 0) {
        precision = ((double) correctly_predicted.size())/predicted_hypothesis.size();
    }
    if (failed_links.size() > 0){
        recall = ((double) correctly_predicted.size())/failed_links.size();
    }
    return PDD(precision, recall);
}
