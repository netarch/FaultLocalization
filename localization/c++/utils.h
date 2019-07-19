#include "flow.h"
#include "linkstats.h"
#include <map>

struct LinkStats{
    int src, dest;
    double drop_rate;
    LinkStats(int src_, int dest_, double drop_rate_): src(src_), dest(dest_), drop_rate(drop_rate_){}
    double GetDropRate () { return drop_rate;}
};

vector<Flow>* GetDataFromLogfile(string filename);

unordered_map<Link, vector<int> >* GetForwardFlowsByLink(
                vector<Flow>* flows, map<Link, int>* inverse_links,
                double max_finish_time_ms);

unordered_map<Link, vector<int> >* GetReverseFlowsByLink(
                vector<Flow>* flows, map<Link, int>* inverse_links,
                double max_finish_time_ms);

unordered_map<Link, vector<int> >* GetFlowsByLink(
                unordered_map<Link, vector<int> >* forward_flows_by_link,
                unordered_map<Link, vector<int> >* reverse_flows_by_link,
                map<Link, int>* inverse_links);

PDD GetPrecisionRecall(set<Link> failed_links, set<Link> predicted_hypothesis);
