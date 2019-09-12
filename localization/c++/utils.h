#ifndef __FAULT_LOCALIZE_UTILS__
#define __FAULT_LOCALIZE_UTILS__

#include "flow.h"
#include <unordered_map>
#include <set>
using namespace std;


struct LogFileData{
    unordered_map<Link, double> failed_links;
    vector<Flow*> flows;
    unordered_map<Link, int> links_to_ids;
    vector<Link> inverse_links;
    vector<vector<int> > *forward_flows_by_link_id, *reverse_flows_by_link_id, *flows_by_link_id;

    LogFileData (): forward_flows_by_link_id(NULL), reverse_flows_by_link_id(NULL), flows_by_link_id(NULL) {}
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
Path* GetReducedPath(Path *path, unordered_map<Link, Link> &reduced_graph_map,
                                 LogFileData &data, LogFileData &reduced_data);
PDD GetPrecisionRecall(Hypothesis& failed_links, Hypothesis& predicted_hypothesis);

#endif
