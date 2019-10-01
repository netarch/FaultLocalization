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
    //long long mem_taken = 0;
    while (getline(infile, line)){
        //const char *linum = line.c_str();
        istringstream line_stream (line);
        //cout << linum <<endl;
        //sscanf(linum, "%s *", op);
        //cout << "op" << op << endl;
        line_stream >> op;
        nlines += 1;
        if (op == "Failing_link"){
            int src, dest;
            float failparam;
            line_stream >> src >> dest >> failparam;
            //sscanf (linum + op.size(),"%d %*d %f", &src, &dest, &failparam);
            result->AddFailedLink(Link(src, dest), failparam);
            if constexpr (VERBOSE){
                cout<< "Failed link "<<src<<" "<<dest<<" "<<failparam<<endl; 
            }
        }
        else if (op == "FID" or op == "Flowid="){
            // Log the previous flow
            if (flow != NULL) flow->DoneAddingPaths();
            if (flow != NULL and flow->paths.size() > 0 and flow->path_taken_vector.size() == 1){
                assert (flow->GetPathTaken());
                if constexpr (CONSIDER_REVERSE_PATH){
                    assert (flow->GetReversePathTaken());
                }
                if (flow->GetLatestPacketsSent() > 0 and !flow->DiscardFlow()
                    and flow->paths.size() > 0){
                    //if (chunk_flows.size() % 10000==0){
                    //    cout << "Finished "<<chunk_flows.size() << " flows from " << filename << endl;
                    //}
                    chunk_flows.push_back(flow);
                }
            }
            int src, dest, nbytes;
            double start_time_ms;
            line_stream >> src >> dest >> nbytes >> start_time_ms;
            flow = new Flow(src, "", 0, dest, "", 0, nbytes, start_time_ms);
            //mem_taken += sizeof(Flow);
        }
        else if (op == "SS" or op == "Snapshot"){
            double snapshot_time_ms;
            int packets_sent, packets_lost, packets_randomly_lost;
            line_stream >> snapshot_time_ms >> packets_sent >> packets_lost >> packets_randomly_lost;
            assert (flow != NULL);
            flow->AddSnapshot(snapshot_time_ms, packets_sent, packets_lost, packets_randomly_lost);
            //mem_taken += sizeof(FlowSnapshot);
        }
        else if (StringStartsWith(op, "FPR") or StringStartsWith(op, "flowpath_reverse")){
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
            flow->SetReverseFirstLinkId(GetLinkId(Link(flow->dest, *path_nodes.begin())));
            flow->SetReverseLastLinkId(GetLinkId(Link(*path_nodes.rbegin(), flow->src)));
            MemoizedPaths *memoized_paths = result->GetMemoizedPaths(path_nodes[0], *path_nodes.rbegin());;
            Path* path = memoized_paths->GetPath(temp_path);
            flow->AddReversePath(path, (op.find("taken")!=string::npos or op.find("T")!=string::npos));
        }
        else if (StringStartsWith(op, "FP") or StringStartsWith(op, "flowpath")){
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
            flow->SetFirstLinkId(GetLinkId(Link(flow->src, *path_nodes.begin())));
            flow->SetLastLinkId(GetLinkId(Link(*path_nodes.rbegin(), flow->dest)));
            MemoizedPaths *memoized_paths = result->GetMemoizedPaths(path_nodes[0], *path_nodes.rbegin());
            Path *path = memoized_paths->GetPath(temp_path);
            flow->AddPath(path, (op.find("taken")!=string::npos or op.find("T")!=string::npos));
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
    //mem_taken += sizeof(chunk_flows) + sizeof(Flow*) * chunk_flows.capacity();
    //cout << "Total space taken by forward paths " << mem_taken/1024 << endl;
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
