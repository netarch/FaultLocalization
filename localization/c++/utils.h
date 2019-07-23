#include "flow.h"
#include "linkstats.h"
#include <map>


struct LogFileData{
    unordered_map<Link, double> failed_links;
    vector<Flow*> flows;
    unordered_map<Link, int> links_to_indices;
    vector<Link> inverse_links;
    unordered_map<Link, vector<int> >* forward_flows_by_link=NULL, reverse_flows_by_link=NULL, flows_by_link=NULL;

    unordered_map<Link, vector<int> >* GetForwardFlowsByLink(double max_finish_time_ms);
    unordered_map<Link, vector<int> >* GetReverseFlowsByLink(double max_finish_time_ms);
    unordered_map<Link, vector<int> >* GetFlowsByLink(double max_finish_time_ms);
};


struct LinkStats{
    int src, dest;
    double drop_rate;
    LinkStats(int src_, int dest_, double drop_rate_): src(src_), dest(dest_), drop_rate(drop_rate_){}
    double GetDropRate () { return drop_rate;}
};

LogFileData* GetDataFromLogfile(string filename);


PDD GetPrecisionRecall(set<Link> failed_links, set<Link> predicted_hypothesis);
