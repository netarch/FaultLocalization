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

MemoizedPaths* LogData::GetMemoizedPaths(int src_rack, int dest_rack){
    PII st(src_rack, dest_rack);
    MemoizedPaths* ret;
    if constexpr (PARALLEL_IO) memoized_paths_lock.lock_shared();
    auto it = memoized_paths.find(st);
    if constexpr (PARALLEL_IO) memoized_paths_lock.unlock_shared();
    if (it == memoized_paths.end()){
        if constexpr (PARALLEL_IO) {
            memoized_paths_lock.lock();
            // need to check a second time for correctness
            it = memoized_paths.find(st);
            if (it == memoized_paths.end()){
                ret = new MemoizedPaths();
                memoized_paths.insert(make_pair(st, ret));
            }
            else{
                ret = it->second;
            }
            memoized_paths_lock.unlock();
        }
        else{
            ret = new MemoizedPaths();
            memoized_paths.insert(make_pair(st, ret));
        }
    }
    else{
        ret = it->second;
    }
    return ret;
}

void LogData::GetReducedData(unordered_map<Link, Link>& reduced_graph_map,
                                 LogData& reduced_data, int nopenmp_threads) {
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

    reduced_data.flows.resize(flows.size());
    atomic<int> flow_ctr = 0;
    #pragma omp parallel for num_threads(nopenmp_threads)
    for (int ff=0; ff<flows.size(); ff++){
        Flow* rf = new Flow(*flows[ff], reduced_graph_map, *this, reduced_data);
        reduced_data.flows[flow_ctr++] = rf;
    }
}


void LogData::FilterFlowsForConditional(double max_finish_time_ms, int nopenmp_threads){
    auto start_filter_time = chrono::high_resolution_clock::now();
    vector<Flow*> filtered_flows[nopenmp_threads];
    #pragma omp parallel for num_threads(nopenmp_threads)
    for (int ff=0; ff<flows.size(); ff++){
        int thread_num = omp_get_thread_num();
        Flow* f = flows[ff];
        if (f->TracerouteFlow(max_finish_time_ms)){
            filtered_flows[thread_num].push_back(f);
            f->ResetSnapshotCounter();
        }
        else{
            delete(f);
        }
    }
    flows.clear();
    for (int t=0; t<nopenmp_threads; t++){
        flows.insert(flows.end(), filtered_flows[t].begin(), filtered_flows[t].end());
    }
    if constexpr (VERBOSE) {
        cout << "Filtered flows for conditional analysis in "
             << GetTimeSinceMilliSeconds(start_filter_time) << " seconds" << endl;
    }
}

vector<vector<int> >* LogData::GetForwardFlowsByLinkId(double max_finish_time_ms, int nopenmp_threads){
    int nlinks = inverse_links.size();
    vector<int> sizes[nopenmp_threads];
    for(int thread_num=0; thread_num<nopenmp_threads; thread_num++){
        sizes[thread_num] = vector<int> (nlinks, 0);
    }
    auto start_time = chrono::high_resolution_clock::now();
    int chunk_size = (flows.size() + nopenmp_threads)/nopenmp_threads; //ceiling
    #pragma omp parallel num_threads(nopenmp_threads)
    {
        int thread_num = omp_get_thread_num();
        int start = min((int)flows.size(), thread_num * chunk_size);
        int end = min((int)flows.size(), (thread_num+1) * chunk_size);
        assert(start <= end);
        for (int ff=start; ff<end; ff++){
            Flow *flow = flows[ff];
            if (flow->AnySnapshotBefore(max_finish_time_ms)){
                auto flow_paths = flow->GetPaths(max_finish_time_ms);
                sizes[thread_num][flow->first_link_id]++;
                sizes[thread_num][flow->last_link_id]++;
                for (Path* path: *flow_paths){
                    for (int link_id: *path){
                        sizes[thread_num][link_id]++;
                    }
                }
            }
        }
    }
    vector<int> final_sizes;
    final_sizes.resize(nlinks);
    #pragma omp parallel for num_threads(nopenmp_threads)
    for (int link_id=0; link_id<nlinks; link_id++){
        // use sizes as offsets
        int curr_bin_size = sizes[0][link_id], last_bin_size;
        sizes[0][link_id] = 0;
        for (int thread_num=1; thread_num<nopenmp_threads; thread_num++){
            last_bin_size = curr_bin_size;
            curr_bin_size = sizes[thread_num][link_id];
            sizes[thread_num][link_id] = sizes[thread_num-1][link_id] + last_bin_size;
        }
        final_sizes[link_id] = sizes[nopenmp_threads-1][link_id] + curr_bin_size;
    }
    if constexpr (VERBOSE){
        cout<<"Binning flows part 1 done in "<<GetTimeSinceMilliSeconds(start_time)<< " seconds"<<endl;
    }
    start_time = chrono::high_resolution_clock::now();
    forward_flows_by_link_id = new vector<vector<int> >(nlinks);
    int nthreads = min(12, nopenmp_threads);
    #pragma omp parallel for num_threads(nthreads)
    for(int link_id=0; link_id<nlinks; link_id++){
        (*forward_flows_by_link_id)[link_id].resize(final_sizes[link_id]);
    }
    if constexpr (VERBOSE){
        cout<<"Binning flows part 2 done in "<<GetTimeSinceMilliSeconds(start_time)<< " seconds"<<endl;
    }
    start_time = chrono::high_resolution_clock::now();
    #pragma omp parallel num_threads(nopenmp_threads)
    {
        int thread_num = omp_get_thread_num();
        int start = min((int)flows.size(), thread_num * chunk_size);
        int end = min((int)flows.size(), (thread_num+1) * chunk_size);
        auto &offsets = sizes[thread_num];
        assert(start <= end);
        for (int ff=start; ff<end; ff++){
            Flow *flow =  flows[ff];
            if (flow->AnySnapshotBefore(max_finish_time_ms)){
                auto flow_paths = flow->GetPaths(max_finish_time_ms);
                (*forward_flows_by_link_id)[flow->first_link_id][offsets[flow->first_link_id]++] = ff;
                (*forward_flows_by_link_id)[flow->last_link_id][offsets[flow->last_link_id]++] = ff;
                for (Path* path: *flow_paths){
                    for (int link_id: *path){
                        (*forward_flows_by_link_id)[link_id][offsets[link_id]++] = ff;
                    }
                }
            }
        }
    }
    if constexpr (VERBOSE){
        cout<<"Binning flows part 3 done in "<<GetTimeSinceMilliSeconds(start_time)<< " seconds"<<endl;
    }
    start_time = chrono::high_resolution_clock::now();
    return forward_flows_by_link_id;
}

vector<vector<int> >* LogData::GetForwardFlowsByLinkId2(double max_finish_time_ms, int nopenmp_threads){
    int nlinks = inverse_links.size();
    forward_flows_by_link_id = new vector<vector<int> >(nlinks);
    mutex link_locks[nlinks];
    #pragma omp parallel for num_threads(nopenmp_threads) schedule(auto)
    for (int ff=0; ff<flows.size(); ff++){
        Flow *flow = flows[ff];
        if (flow->AnySnapshotBefore(max_finish_time_ms)){
            // src --> src_tor
            link_locks[flow->first_link_id].lock();
            (*forward_flows_by_link_id)[flow->first_link_id].push_back(ff);
            link_locks[flow->first_link_id].unlock();

            // dest_tor --> dest
            link_locks[flow->last_link_id].lock();
            (*forward_flows_by_link_id)[flow->last_link_id].push_back(ff);
            link_locks[flow->last_link_id].unlock();

            auto flow_paths = flow->GetPaths(max_finish_time_ms);
            for (Path* path: *flow_paths){
                for (int link_id: *path){
                    link_locks[link_id].lock();
                    (*forward_flows_by_link_id)[link_id].push_back(ff);
                    link_locks[link_id].unlock();
                }
            }
        }
    }
    return forward_flows_by_link_id;
}

void LogData::GetSizesForForwardFlowsByLinkId(double max_finish_time_ms, int nopenmp_threads, vector<int>& result){
    int nlinks = inverse_links.size();
    vector<int> sizes[nopenmp_threads];
    for(int thread_num=0; thread_num<nopenmp_threads; thread_num++){
        sizes[thread_num] = vector<int> (nlinks, 0);
    }
    #pragma omp parallel for num_threads(nopenmp_threads)
    for (int ff=0; ff<flows.size(); ff++){
        Flow *flow = flows[ff];
        if (flow->AnySnapshotBefore(max_finish_time_ms)){
            auto flow_paths = flow->GetPaths(max_finish_time_ms);
            int thread_num = omp_get_thread_num();
            sizes[thread_num][flow->first_link_id]++;
            sizes[thread_num][flow->last_link_id]++;
            for (Path* path: *flow_paths){
                for (int link_id: *path){
                    sizes[thread_num][link_id]++;
                }
            }
        }
    }
    result.reserve(nlinks);
    for (int link_id=0; link_id<nlinks; link_id++){
        result.push_back(0);
        for (int thread_num=0; thread_num<nopenmp_threads; thread_num++){
            result[link_id] += sizes[thread_num][link_id];
        }
    }
}

// Lockless version
vector<vector<int> >* LogData::GetForwardFlowsByLinkId1(double max_finish_time_ms, int nopenmp_threads){
    int nlinks = inverse_links.size();
    forward_flows_by_link_id = new vector<vector<int> >(nlinks);
    auto start_time = chrono::high_resolution_clock::now();
    vector<int> sizes;
    GetSizesForForwardFlowsByLinkId(max_finish_time_ms, nopenmp_threads, sizes);
    if constexpr (VERBOSE){
        cout<<"Binning flows part 1 done in "<<GetTimeSinceMilliSeconds(start_time)<< " seconds"<<endl;
    }
    start_time = chrono::high_resolution_clock::now();
    int nthreads = min(3, nopenmp_threads);
    #pragma omp parallel for num_threads(nthreads)
    for(int link_id=0; link_id<nlinks; link_id++){
        (*forward_flows_by_link_id)[link_id].reserve(sizes[link_id]);
    }
    if constexpr (VERBOSE){
        cout<<"Binning flows part 2 done in "<<GetTimeSinceMilliSeconds(start_time)<< " seconds"<<endl;
    }
    start_time = chrono::high_resolution_clock::now();
    //mutex link_locks[nlinks];
    atomic_flag link_locks[nlinks](ATOMIC_FLAG_INIT);
    #pragma omp parallel for num_threads(nopenmp_threads) schedule(auto)
    for (int ff=0; ff<flows.size(); ff++){
        Flow *flow =  flows[ff];
        if (flow->AnySnapshotBefore(max_finish_time_ms)){
            auto flow_paths = flow->GetPaths(max_finish_time_ms);

            //link_locks[flow->first_link_id].lock();
            while (link_locks[flow->first_link_id].test_and_set(std::memory_order_acquire));
            (*forward_flows_by_link_id)[flow->first_link_id].push_back(ff);
            link_locks[flow->first_link_id].clear(std::memory_order_release);
            //link_locks[flow->first_link_id].unlock();

            //link_locks[flow->last_link_id].lock();
            while (link_locks[flow->last_link_id].test_and_set(std::memory_order_acquire));
            (*forward_flows_by_link_id)[flow->last_link_id].push_back(ff);
            link_locks[flow->last_link_id].clear(std::memory_order_release);
            //link_locks[flow->last_link_id].unlock();

            for (Path* path: *flow_paths){
                for (int link_id: *path){
                    //link_locks[link_id].lock();
                    while (link_locks[link_id].test_and_set(std::memory_order_acquire));
                    (*forward_flows_by_link_id)[link_id].push_back(ff);
                    link_locks[link_id].clear(std::memory_order_release);
                    //link_locks[link_id].unlock();
                }
            }
        }
    }
    if constexpr (VERBOSE){
        cout<<"Binning flows part 3 done in "<<GetTimeSinceMilliSeconds(start_time)<< " seconds"<<endl;
    }
    start_time = chrono::high_resolution_clock::now();
    return forward_flows_by_link_id;
}

vector<vector<int> >* LogData::GetReverseFlowsByLinkId(double max_finish_time_ms){
    if (reverse_flows_by_link_id != NULL) delete(reverse_flows_by_link_id);
    reverse_flows_by_link_id = new vector<vector<int> >(inverse_links.size());
    for (int ff=0; ff<flows.size(); ff++){
        Flow *flow = flows[ff];
        if (flow->AnySnapshotBefore(max_finish_time_ms)){
            auto flow_reverse_paths = flow->GetReversePaths(max_finish_time_ms);
            (*reverse_flows_by_link_id)[flow->reverse_first_link_id].push_back(ff);
            (*reverse_flows_by_link_id)[flow->reverse_last_link_id].push_back(ff);
            for (Path* path: *flow_reverse_paths){
                for (int link_id: *path){
                    (*reverse_flows_by_link_id)[link_id].push_back(ff);
                }
            }
        }
    }
    return reverse_flows_by_link_id;
}

vector<vector<int> >* LogData::GetFlowsByLinkId(double max_finish_time_ms, int nopenmp_threads){
    GetForwardFlowsByLinkId(max_finish_time_ms, nopenmp_threads);
    if constexpr (!CONSIDER_REVERSE_PATH) flows_by_link_id = forward_flows_by_link_id;
    else{
        assert(false);
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
        delete(forward_flows_by_link_id);
        delete(reverse_flows_by_link_id);
    }
    return flows_by_link_id;
}

void LogData::GetFailedLinkIds(Hypothesis &failed_links_set){
    for (auto &it: failed_links){
        failed_links_set.insert(links_to_ids[it.first]);
    } 
}

set<Link> LogData::IdsToLinks(Hypothesis &h){
    set<Link> ret;
    for (int link_id: h){
        ret.insert(inverse_links[link_id]);
    }
    return ret;
}

void LogData::AddChunkFlows(vector<Flow*> &chunk_flows){
    #pragma omp critical (AddChunkFlows)
    {
        flows.insert(flows.end(), chunk_flows.begin(), chunk_flows.end());
    }
}
        
void LogData::AddFailedLink(Link link, double failparam){
    #pragma omp critical (AddFailedLink)
    {
        failed_links[link] = failparam;
    }
}
        
int LogData::GetLinkId(Link link){
    int link_id;
    #pragma omp critical (AddNewLink)
    {
        auto it = links_to_ids.find(link);
        if (it == links_to_ids.end()){
            link_id = inverse_links.size();
            links_to_ids[link] = link_id;
            inverse_links.push_back(link);
        }
        else link_id = it->second;
    }
    return link_id;
}


int GetReducedLinkId(int link_id, unordered_map<Link, Link> &reduced_graph_map,
                                  LogData &data, LogData &reduced_data){
    Link l = data.inverse_links[link_id];
    Link rl = (reduced_graph_map.find(l) == reduced_graph_map.end()? l: reduced_graph_map[l]);
    int link_id_r = reduced_data.links_to_ids[rl];
    return link_id_r;
}

void GetNumReducedLinksMap(unordered_map<Link, Link> &reduced_graph_map,
                                LogData &data, LogData &reduced_data,
                                unordered_map<int, int> &result_map){
    for (int link_id=0; link_id<data.inverse_links.size(); link_id++){
        result_map[GetReducedLinkId(link_id, reduced_graph_map, data, reduced_data)]++;
    }
}

Path* GetReducedPath(Path *path, unordered_map<Link, Link> &reduced_graph_map,
                                            LogData &data, LogData &reduced_data){
    Path *reduced_path = new Path(path->size());
    // convert link_ids in path to reduced link_ids
    for (int link_id: *path){
        reduced_path->push_back(GetReducedLinkId(link_id, reduced_graph_map, data, reduced_data));
    }
    return reduced_path;
}
