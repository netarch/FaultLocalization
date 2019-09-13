#include "utils.h"
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

inline bool string_starts_with(const string &a, const string &b) {
    return (b.length() <= a.length() && equal(b.begin(), b.end(), a.begin()));
}

LogFileData* GetDataFromLogFileDistributed(string dirname, int nchunks, int nopenmp_threads){ 
    auto start_time = chrono::high_resolution_clock::now();
    LogFileData* all_data = new LogFileData();
    #pragma omp parallel for num_threads(nopenmp_threads)
    for(int i=0; i<nchunks; i++){
        GetDataFromLogFile(dirname + "/" + to_string(i), all_data);
    }
    if (VERBOSE){
        cout<<"Read log file chunks in "<<chrono::duration_cast<chrono::seconds>(
            chrono::high_resolution_clock::now() - start_time).count() << " seconds "<< endl;
    }
    return all_data;
}

void GetDataFromLogFile(string filename, LogFileData *result){ 
    auto start_time = chrono::high_resolution_clock::now();
    vector<Flow*> chunk_flows;
    //unordered_map<Link, int> links_to_ids_cache;
    dense_hash_map<Link, int, hash<Link> > links_to_ids_cache;
    links_to_ids_cache.set_empty_key(Link(-1, -1));
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
            if (VERBOSE){
                cout<< "Failed link "<<src<<" "<<dest<<" "<<failparam<<endl; 
            }
        }
        else if (op == "Flowid="){
            // Log the previous flow
            if (flow != NULL) flow->DoneAddingPaths();
            if (flow != NULL and flow->paths.size() > 0 and flow->path_taken_vector.size() == 1){
                assert (flow->GetPathTaken());
                if (CONSIDER_REVERSE_PATH){
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
        else if (op == "Snapshot"){
            double snapshot_time_ms;
            int packets_sent, packets_lost, packets_randomly_lost;
            line_stream >> snapshot_time_ms >> packets_sent >> packets_lost >> packets_randomly_lost;
            assert (flow != NULL);
            flow->AddSnapshot(snapshot_time_ms, packets_sent, packets_lost, packets_randomly_lost);
            //mem_taken += sizeof(FlowSnapshot);
        }
        else if (string_starts_with(op, "flowpath_reverse")){
            temp_path.clear();
            path_nodes.clear();
            int prev_node, node, nodenum=0;
            while (line_stream >> node){
                if (nodenum > 0){
                    Link link(prev_node, node);
                    int link_id;
                    auto it = links_to_ids_cache.find(link);
                    if (it == links_to_ids_cache.end()){
                        link_id = result->GetLinkId(link);
                        links_to_ids_cache[link] = link_id;
                    }
                    else{
                        link_id = it->second;
                    }
                    temp_path.push_back(link_id);
                }
                path_nodes.push_back(node);
                prev_node = node;
                ++nodenum;
            }
            if ((MEMOIZE_PATHS and temp_path.size() < 2) or (!MEMOIZE_PATHS and temp_path.size() == 0)){
                continue;
            }
            Path* path;
            if (MEMOIZE_PATHS) {
                flow->SetReverseFirstLinkId(temp_path.front());
                flow->SetReverseLastLinkId(temp_path.back());
                temp_path.erase(temp_path.begin());
                temp_path.pop_back();
                assert (path_nodes.size() >= 2);
                MemoizedPaths *memoized_paths;
                PII st(path_nodes[1], *(path_nodes.rbegin()-1));
                result->memoized_paths_lock.lock_shared();
                auto it = result->memoized_paths.find(st);
                result->memoized_paths_lock.unlock_shared();
                if (it == result->memoized_paths.end()){
                    result->memoized_paths_lock.lock();
                    // need to check a second time for correctness
                    it = result->memoized_paths.find(st);
                    if (it == result->memoized_paths.end()){
                        memoized_paths = new MemoizedPaths();
                        result->memoized_paths.insert(make_pair(st, memoized_paths));
                    }
                    else{
                        memoized_paths = it->second;
                    }
                    result->memoized_paths_lock.unlock();
                }
                else{
                    memoized_paths = it->second;
                }
                path = memoized_paths->GetPath(temp_path);
            }
            else{
                path = new Path(temp_path);
            }
            //path->shrink_to_fit();
            assert (flow != NULL);
            flow->AddReversePath(path, (op.find("taken") != string::npos));
        }
        else if (string_starts_with(op, "flowpath")){
            temp_path.clear();
            path_nodes.clear();
            int prev_node, node, nodenum=0;
            while (line_stream >> node){
                if (nodenum > 0){
                    Link link(prev_node, node);
                    int link_id;
                    auto it = links_to_ids_cache.find(link);
                    if (it == links_to_ids_cache.end()){
                        link_id = result->GetLinkId(link);
                        links_to_ids_cache[link] = link_id;
                    }
                    else{
                        link_id = it->second;
                    }
                    temp_path.push_back(link_id);
                }
                path_nodes.push_back(node);
                prev_node = node;
                ++nodenum;
            }
            if ((MEMOIZE_PATHS and temp_path.size() < 2) or (!MEMOIZE_PATHS and temp_path.size()== 0)){
                continue;
            }
            Path* path;
            if (MEMOIZE_PATHS) {
                flow->SetFirstLinkId(temp_path.front());
                flow->SetLastLinkId(temp_path.back());
                temp_path.erase(temp_path.begin());
                temp_path.pop_back();
                assert (path_nodes.size() >= 2);
                MemoizedPaths *memoized_paths;
                PII st(path_nodes[1], *(path_nodes.rbegin()-1));
                result->memoized_paths_lock.lock_shared();
                auto it = result->memoized_paths.find(st);
                result->memoized_paths_lock.unlock_shared();
                if (it == result->memoized_paths.end()){
                    result->memoized_paths_lock.lock();
                    // need to check a second time for correctness
                    it = result->memoized_paths.find(st);
                    if (it == result->memoized_paths.end()){
                        memoized_paths = new MemoizedPaths();
                        result->memoized_paths.insert(make_pair(st, memoized_paths));
                    }
                    else{
                        memoized_paths = it->second;
                    }
                    result->memoized_paths_lock.unlock();
                }
                else{
                    memoized_paths = it->second;
                }
                path = memoized_paths->GetPath(temp_path);
            }
            else{
                path = new Path(temp_path);
            }
            //mem_taken += sizeof(*path) + sizeof(int) * path->size();
            assert (flow != NULL);
            flow->AddPath(path, (op.find("taken") != string::npos));
        }
        else if (op == "link_statistics"){
            assert (false);
        }
    }
    // Log the last flow
    if (flow != NULL) flow->DoneAddingPaths();
    if (flow != NULL and flow->paths.size() > 0 and flow->path_taken_vector.size() == 1){
        assert (flow->GetPathTaken());
        if (CONSIDER_REVERSE_PATH){
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
    if (VERBOSE){
        cout<<"Read log file in "<<chrono::duration_cast<chrono::milliseconds>(
            chrono::high_resolution_clock::now() - start_time).count()/1000.0 << " seconds, numlines " << nlines << endl;
    }
}


void LogFileData::GetReducedData(unordered_map<Link, Link>& reduced_graph_map,
                                 LogFileData& reduced_data, int nopenmp_threads) {
    for (auto& it: failed_links){
        Link l = it.first;
        Link rl = (reduced_graph_map.find(l) == reduced_graph_map.end()? l: reduced_graph_map[l]);
        if (reduced_data.failed_links.find(rl) == reduced_data.failed_links.end()){
            //!HACK: set dummy failparam
            reduced_data.failed_links.insert(make_pair(rl, 0.0));
        }
    }
    for (auto& it: reduced_data.failed_links){
        cout << "Failed reduced link "<< it.first << endl;
    }

    for(auto &it: links_to_ids){
        Link l = it.first;
        Link rl = (reduced_graph_map.find(l) == reduced_graph_map.end()? l: reduced_graph_map[l]);
        if (reduced_data.links_to_ids.find(rl) == reduced_data.links_to_ids.end()){
            reduced_data.links_to_ids[rl] = reduced_data.inverse_links.size();
            reduced_data.inverse_links.push_back(rl);
        }
    }
    cout << "Finished assigning ids to reduced links, nlinks(reduced) " << reduced_data.inverse_links.size() << endl;

    //mutex lock;
    reduced_data.flows.resize(flows.size());
    atomic<int> flow_ctr = 0;
    #pragma omp parallel for num_threads(nopenmp_threads)
    for (int ff=0; ff<flows.size(); ff++){
        Flow* rf = new Flow(*flows[ff], reduced_graph_map, *this, reduced_data);
        //lock.lock();
        reduced_data.flows[flow_ctr++] = rf;
        //reduced_data.flows.push_back(rf); 
        //lock.unlock();
    }
}


void LogFileData::FilterFlowsForConditional(double max_finish_time_ms, int nopenmp_threads){
    vector<Flow*> filtered_flows[nopenmp_threads];
    #pragma omp parallel for num_threads(nopenmp_threads)
    for (int ff=0; ff<flows.size(); ff++){
        int thread_num = omp_get_thread_num();
        Flow* f = flows[ff];
        if (f->TracerouteFlow(max_finish_time_ms)){
            filtered_flows[thread_num].push_back(f);
        }
        else{
            delete(f);
        }
    }
    flows.clear();
    for (int t=0; t<nopenmp_threads; t++){
        for (Flow* f: filtered_flows[t]){
            flows.push_back(f);
        }
    }
}

vector<vector<int> >* LogFileData::GetForwardFlowsByLinkId(double max_finish_time_ms, int nopenmp_threads){
    if (forward_flows_by_link_id != NULL) delete(forward_flows_by_link_id);
    int nlinks = inverse_links.size();
    forward_flows_by_link_id = new vector<vector<int> >(nlinks);
    mutex link_locks[nlinks];
    #pragma omp parallel for num_threads(nopenmp_threads)
    for (int ff=0; ff<flows.size(); ff++){
        Flow *flow = flows[ff];
        auto flow_paths = flow->GetPaths(max_finish_time_ms);
        if (MEMOIZE_PATHS){
            // src --> src_tor
            link_locks[flow->first_link_id].lock();
            (*forward_flows_by_link_id)[flow->first_link_id].push_back(ff);
            link_locks[flow->first_link_id].unlock();
            // dest_tor --> dest
            link_locks[flow->last_link_id].lock();
            (*forward_flows_by_link_id)[flow->last_link_id].push_back(ff);
            link_locks[flow->last_link_id].unlock();
        }
        for (Path* path: *flow_paths){
            for (int link_id: *path){
                link_locks[link_id].lock();
                (*forward_flows_by_link_id)[link_id].push_back(ff);
                link_locks[link_id].unlock();
            }
        }
    }
    return forward_flows_by_link_id;
}

//Lockless version
vector<vector<int> >* LogFileData::GetForwardFlowsByLinkId1(double max_finish_time_ms, int nopenmp_threads){
    if (forward_flows_by_link_id != NULL) delete(forward_flows_by_link_id);
    int nlinks = inverse_links.size();
    forward_flows_by_link_id = new vector<vector<int> >(nlinks);
    atomic<unsigned int> sizes[nlinks];
    auto start_time = chrono::high_resolution_clock::now();
    #pragma omp parallel for num_threads(nopenmp_threads)
    for(int link_id=0; link_id<nlinks; link_id++){
        sizes[link_id] = 0;
    }
    if (VERBOSE){
        cout<<"Binning flows part 0 done in "<<chrono::duration_cast<chrono::milliseconds>(
            chrono::high_resolution_clock::now() - start_time).count()*1.0e-3 << " seconds "<< endl;
    }
    start_time = chrono::high_resolution_clock::now();
    #pragma omp parallel for num_threads(nopenmp_threads)
    for (int ff=0; ff<flows.size(); ff++){
        Flow *flow = flows[ff];
        auto flow_paths = flow->GetPaths(max_finish_time_ms);
        if (MEMOIZE_PATHS){
            sizes[flow->first_link_id]++;
            sizes[flow->last_link_id]++;
        }
        for (Path* path: *flow_paths){
            for (int link_id: *path){
                sizes[link_id]++;
            }
        }
    }
    if (VERBOSE){
        cout<<"Binning flows part 1 done in "<<chrono::duration_cast<chrono::milliseconds>(
            chrono::high_resolution_clock::now() - start_time).count()*1.0e-3 << " seconds "<< endl;
    }
    start_time = chrono::high_resolution_clock::now();
    int nthreads = min(3, nopenmp_threads);
    #pragma omp parallel for num_threads(nthreads)
    for(int link_id=0; link_id<nlinks; link_id++){
        (*forward_flows_by_link_id)[link_id].resize(sizes[link_id]);
    }
    if (VERBOSE){
        cout<<"Binning flows part 2 done in "<<chrono::duration_cast<chrono::milliseconds>(
            chrono::high_resolution_clock::now() - start_time).count()*1.0e-3 << " seconds "<< endl;
    }
    start_time = chrono::high_resolution_clock::now();
    #pragma omp parallel for num_threads(nopenmp_threads)
    for (int ff=0; ff<flows.size(); ff++){
        Flow *flow =  flows[ff];
        auto flow_paths = flow->GetPaths(max_finish_time_ms);
        if (MEMOIZE_PATHS){
            (*forward_flows_by_link_id)[flow->first_link_id][--sizes[flow->first_link_id]] = ff;
            (*forward_flows_by_link_id)[flow->last_link_id][--sizes[flow->last_link_id]] = ff;
        }
        for (Path* path: *flow_paths){
            for (int link_id: *path){
                (*forward_flows_by_link_id)[link_id][--sizes[link_id]] = ff;
            }
        }
    }
    if (VERBOSE){
        cout<<"Binning flows part 3 done in "<<chrono::duration_cast<chrono::milliseconds>(
            chrono::high_resolution_clock::now() - start_time).count()*1.0e-3 << " seconds "<< endl;
    }
    start_time = chrono::high_resolution_clock::now();
    return forward_flows_by_link_id;
}

vector<vector<int> >* LogFileData::GetReverseFlowsByLinkId(double max_finish_time_ms){
    if (reverse_flows_by_link_id != NULL) delete(reverse_flows_by_link_id);
    reverse_flows_by_link_id = new vector<vector<int> >(inverse_links.size());
    for (int ff=0; ff<flows.size(); ff++){
        Flow *flow = flows[ff];
        auto flow_reverse_paths = flow->GetReversePaths(max_finish_time_ms);
        if (MEMOIZE_PATHS){
            (*reverse_flows_by_link_id)[flow->reverse_first_link_id].push_back(ff);
            (*reverse_flows_by_link_id)[flow->reverse_last_link_id].push_back(ff);
        }
        for (Path* path: *flow_reverse_paths){
            for (int link_id: *path){
                (*reverse_flows_by_link_id)[link_id].push_back(ff);
            }
        }
    }
    return reverse_flows_by_link_id;
}

vector<vector<int> >* LogFileData::GetFlowsByLinkId(double max_finish_time_ms, int nopenmp_threads){
    if (flows_by_link_id != NULL) delete(flows_by_link_id);
    GetForwardFlowsByLinkId(max_finish_time_ms, nopenmp_threads);
    if (!CONSIDER_REVERSE_PATH) flows_by_link_id = forward_flows_by_link_id;
    else{
        GetReverseFlowsByLinkId(max_finish_time_ms);
        flows_by_link_id = new vector<vector<int> >(inverse_links.size());
        for (int link_id=0; link_id<inverse_links.size(); link_id++){
            set_union(forward_flows_by_link_id->at(link_id).begin(),
                      forward_flows_by_link_id->at(link_id).end(),
                      reverse_flows_by_link_id->at(link_id).begin(),
                      reverse_flows_by_link_id->at(link_id).end(),
                      back_inserter(flows_by_link_id->at(link_id)));
            //!TODO: Check if flows_by_link actually gets populated
        }
    }
    return flows_by_link_id;
}

void LogFileData::GetFailedLinkIds(Hypothesis &failed_links_set){
    for (auto &it: failed_links){
        failed_links_set.insert(links_to_ids[it.first]);
    } 
}

set<Link> LogFileData::IdsToLinks(Hypothesis &h){
    set<Link> ret;
    for (int link_id: h){
        ret.insert(inverse_links[link_id]);
    }
    return ret;
}

void LogFileData::AddChunkFlows(vector<Flow*> &chunk_flows){
    #pragma omp critical (AddChunkFlows)
    {
        flows.insert(flows.end(), chunk_flows.begin(), chunk_flows.end());
    }
}
        
void LogFileData::AddFailedLink(Link link, double failparam){
    #pragma omp critical (AddFailedLink)
    {
        failed_links[link] = failparam;
    }
}
        
int LogFileData::GetLinkId(Link link){
    int ret;
    #pragma omp critical (AddNewLink)
    {
        if (links_to_ids.find(link) == links_to_ids.end()){
                links_to_ids[link] = inverse_links.size();
                inverse_links.push_back(link);
        }
        ret = links_to_ids[link];
    }
    return ret;
}

Path* GetReducedPath(Path *path, unordered_map<Link, Link> &reduced_graph_map,
                                            LogFileData &data, LogFileData &reduced_data){
    Path *reduced_path = new Path(path->size());
    // convert link_ids in path to reduced link_ids
    for (int link_id: *path){
        Link l = data.inverse_links[link_id];
        Link rl = (reduced_graph_map.find(l) == reduced_graph_map.end()? l: reduced_graph_map[l]);
        int link_id_r = reduced_data.links_to_ids[rl];
        reduced_path->push_back(link_id_r);
    }
    return reduced_path;
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

