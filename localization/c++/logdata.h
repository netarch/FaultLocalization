#ifndef __LOG_DATA__
#define __LOG_DATA__

#include "flow.h"
#include "utils.h"
#include <map>
#include <unordered_map>
#include <set>
#include <mutex>
#include <shared_mutex>
#include <sparsehash/dense_hash_map>
#include <fstream>
using google::dense_hash_map;
using namespace std;

class LogData{
public:
    dense_hash_map<Link, double, hash<Link> > failed_links;
    dense_hash_map<int, double, hash<int> > failed_devices;
    vector<Flow*> flows;
    dense_hash_map<int, int, hash<int> > hosts_to_racks;
    //dense_hash_map<Link, int, hash<Link> > links_to_ids;
    map<Link, int> links_to_ids;
    vector<Link> inverse_links;
    vector<vector<int> > *forward_flows_by_link_id, *reverse_flows_by_link_id, *flows_by_link_id;
    vector<vector<int> > *flows_by_device;
    //dense_hash_map<PII, MemoizedPaths*, hash<PII> > memoized_paths;
    unordered_map<PII, MemoizedPaths*> memoized_paths;
    shared_mutex memoized_paths_lock;
    MemoizedPaths* GetMemoizedPaths(int src_rack, int dest_rack);

    LogData (LogData &data);
    LogData (): forward_flows_by_link_id(NULL), reverse_flows_by_link_id(NULL), flows_by_link_id(NULL) {
        failed_links.set_empty_key(Link(-1, -1));
        failed_devices.set_empty_key(-1);
        hosts_to_racks.set_empty_key(-1);
        //links_to_ids.set_empty_key(Link(-1, -1));
        //memoized_paths.set_empty_key(PII(-1, -1));
    }

    void GetDeviceLevelPath(Path &path, Path &result);

    vector<vector<int> >* GetForwardFlowsByLinkId(double max_finish_time_ms, int nopenmp_threads);
    void GetSizesForForwardFlowsByLinkId(double max_finish_time_ms, int nopenmp_threads, vector<int>& result);
    vector<vector<int> >* GetForwardFlowsByLinkId1(double max_finish_time_ms, int nopenmp_threads);
    vector<vector<int> >* GetForwardFlowsByLinkId2(double max_finish_time_ms, int nopenmp_threads);
    vector<vector<int> >* GetReverseFlowsByLinkId(double max_finish_time_ms);
    vector<vector<int> >* GetFlowsByLinkId(double max_finish_time_ms, int nopenmp_threads);

    vector<vector<int> >* GetFlowsByDevice(double max_finish_time_ms, int nopenmp_threads);
    void GetFailedDevices(Hypothesis &failed_devices_set);

    void GetFailedLinkIds(Hypothesis &failed_links_set);
    void FilterFlowsForConditional(double max_finish_time_ms, int nopenmp_threads);
    void FilterFlowsBeforeTime(double finish_time_ms, int nopenmp_threads);

    int GetLinkId(Link link);
    int GetLinkIdUnsafe(Link link);
    void AddChunkFlows(vector<Flow*> &chunk_flows);
    void AddFailedLink(Link link, double failparam);

    void GetRacksList(vector<int> &result);
    set<Link> IdsToLinks(Hypothesis &h);

    int GetMaxDevicePlus1();

    void GetReducedData(unordered_map<Link, Link>& reduced_graph_map, LogData& reduced_data,
                        int nopenmp_threads);

    void ResetForAnalysis();
    Path* GetPointerToPathTaken(vector<int>& path_nodes, vector<int>& temp_path, Flow* flow);
    void GetAllPaths(vector<Path*> **result, int src_rack, int dest_rack);

    void OutputToTrace(ostream &out);
    vector<Flow*> flow_pointers_to_delete;
    void CleanupFlows();
    int max_device = -1;
    ~LogData();
};

/* For reduced analysis */
int GetReducedLinkId(int link_id, unordered_map<Link, Link> &reduced_graph_map,
                                  LogData &data, LogData &reduced_data);
Path* GetReducedPath(Path *path, unordered_map<Link, Link> &reduced_graph_map,
                                 LogData &data, LogData &reduced_data);
void GetNumReducedLinksMap(unordered_map<Link, Link> &reduced_graph_map,
                                LogData &data, LogData &reduced_data,
                                unordered_map<int, int> &result_map);
#endif
