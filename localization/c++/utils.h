#ifndef __FAULT_LOCALIZE_UTILS__
#define __FAULT_LOCALIZE_UTILS__

#include "flow.h"
#include <unordered_map>
#include <set>
using namespace std;

struct LogFileData{
    unordered_map<Link, double> failed_links;
    vector<Flow*> flows;
    unordered_map<Link, int> links_to_indices;
    vector<Link> inverse_links;
    unordered_map<Link, vector<int> > *forward_flows_by_link, *reverse_flows_by_link, *flows_by_link;

    LogFileData (): forward_flows_by_link(NULL), reverse_flows_by_link(NULL), flows_by_link(NULL) {}
    unordered_map<Link, vector<int> >* GetForwardFlowsByLink(double max_finish_time_ms);
    unordered_map<Link, vector<int> >* GetReverseFlowsByLink(double max_finish_time_ms);
    unordered_map<Link, vector<int> >* GetFlowsByLink(double max_finish_time_ms);
    void GetFailedLinksSet(Hypothesis &failed_links_set);
    void FilterFlowsForConditional(double max_finish_time_ms);

    void AddChunkData(LogFileData* chunk_data);
};

struct LinkStats{
    int src, dest;
    double drop_rate;
    LinkStats(int src_, int dest_, double drop_rate_): src(src_), dest(dest_), drop_rate(drop_rate_){}
    double GetDropRate () { return drop_rate;}
};

LogFileData* GetDataFromLogFile(string filename);
LogFileData* GetDataFromLogFileDistributed(string dirname, int nchunks, int nopenmp_threads);

PDD GetPrecisionRecall(Hypothesis& failed_links, Hypothesis& predicted_hypothesis);

#endif
