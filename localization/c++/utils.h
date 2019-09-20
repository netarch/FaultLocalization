#ifndef __FAULT_LOCALIZE_UTILS__
#define __FAULT_LOCALIZE_UTILS__

#include "flow.h"
#include <map>
#include <unordered_map>
#include <set>
#include <mutex>
#include <shared_mutex>
#include <sparsehash/dense_hash_map>
using google::dense_hash_map;
using namespace std;

struct MemoizedPaths{
    dense_hash_map<int, vector<Path*> > paths_by_first_node;
    shared_mutex lock;
    MemoizedPaths() {
        paths_by_first_node.set_empty_key(-1000000);
    }
    Path* GetPath(vector<int>& vi_path){
        int first_node = (vi_path.size()>0? vi_path[0] : -1);
        lock.lock();
        vector<Path*> &paths = paths_by_first_node[first_node];
        Path* ret = NULL;
        for (Path* path: paths){
            if (*path == vi_path){
                ret = path;
                break;
            }
        }
        if (ret == NULL){
            ret = new Path(vi_path);
            paths.push_back(ret);
        }
        lock.unlock();
        return ret; 
    }
};

struct LogFileData{
    dense_hash_map<Link, double, hash<Link> > failed_links;
    vector<Flow*> flows;
    dense_hash_map<Link, int, hash<Link> > links_to_ids;
    vector<Link> inverse_links;
    vector<vector<int> > *forward_flows_by_link_id, *reverse_flows_by_link_id, *flows_by_link_id;
    dense_hash_map<PII, MemoizedPaths*, hash<PII> > memoized_paths;
    shared_mutex memoized_paths_lock;

    LogFileData (): forward_flows_by_link_id(NULL), reverse_flows_by_link_id(NULL), flows_by_link_id(NULL) {
        failed_links.set_empty_key(Link(-1, -1));
        links_to_ids.set_empty_key(Link(-1, -1));
        memoized_paths.set_empty_key(PII(-1, -1));
        
    }
    vector<vector<int> >* GetForwardFlowsByLinkId(double max_finish_time_ms, int nopenmp_threads);
    vector<vector<int> >* GetForwardFlowsByLinkId1(double max_finish_time_ms, int nopenmp_threads);
    vector<vector<int> >* GetReverseFlowsByLinkId(double max_finish_time_ms);
    vector<vector<int> >* GetFlowsByLinkId(double max_finish_time_ms, int nopenmp_threads);
    void GetFailedLinkIds(Hypothesis &failed_links_set);
    void FilterFlowsForConditional(double max_finish_time_ms, int nopenmp_threads);

    int GetLinkId(Link link);
    void AddChunkFlows(vector<Flow*> &chunk_flows);
    void AddFailedLink(Link link, double failparam);

    set<Link> IdsToLinks(Hypothesis &h);

    void GetReducedData(unordered_map<Link, Link>& reduced_graph_map, LogFileData& reduced_data,
                        int nopenmp_threads);

};

void GetDataFromLogFile(string filename, LogFileData* result);
LogFileData* GetDataFromLogFileDistributed(string dirname, int nchunks, int nopenmp_threads);
PDD GetPrecisionRecall(Hypothesis& failed_links, Hypothesis& predicted_hypothesis);

/* For reduced analysis */
int GetReducedLinkId(int link_id, unordered_map<Link, Link> &reduced_graph_map,
                                  LogFileData &data, LogFileData &reduced_data);
Path* GetReducedPath(Path *path, unordered_map<Link, Link> &reduced_graph_map,
                                 LogFileData &data, LogFileData &reduced_data);
void GetNumReducedLinksMap(unordered_map<Link, Link> &reduced_graph_map,
                                LogFileData &data, LogFileData &reduced_data,
                                unordered_map<int, int> &result_map);

#endif
