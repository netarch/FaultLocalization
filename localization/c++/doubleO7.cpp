#include "doubleO7.h"
#include "utils.h"
#include <assert.h>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <cmath>
#include <iostream>

using namespace std;

void DoubleO7::SetLogData(LogData* data_, double max_finish_time_ms, int nopenmp_threads){
    Estimator::SetLogData(data_, max_finish_time_ms, nopenmp_threads);
    data->FilterFlowsForConditional(max_finish_time_ms, nopenmp_threads);
}

DoubleO7* DoubleO7::CreateObject() {
    DoubleO7* ret = new DoubleO7();
    vector<double> param {fail_threshold};
    ret->SetParams(param);
    return ret;
}
    
void DoubleO7::SetParams(vector<double>& params){
    assert(params.size() == 1);
    fail_threshold = params[0];
}

PDD DoubleO7::ComputeVotes(vector<Flow*>& bad_flows, vector<double>& votes,
                           Hypothesis &problematic_link_ids, double min_start_time_ms,
                           double max_finish_time_ms){
    fill(votes.begin(), votes.end(), 0.0);
    for(int ff=0; ff < bad_flows.size(); ff++){
        Flow *flow = bad_flows[ff];
        assert(flow->TracerouteFlow(max_finish_time_ms));
        Path *path_taken = flow->GetPathTaken();
        if (!HypothesisIntersectsPath(&problematic_link_ids, path_taken) and
            (problematic_link_ids.count(flow->first_link_id) == 0) and
            (problematic_link_ids.count(flow->last_link_id) == 0)){
            int flow_vote = (int) (flow->LabelWeightsFunc(max_finish_time_ms).second > 0);
            assert(flow_vote == 1);
            int total_path_length = path_taken->size() + 2;
            double vote = ((double)flow_vote) / total_path_length;
            votes[flow->first_link_id] += vote;
            votes[flow->last_link_id] += vote;
            for (int link_id: *path_taken){
                votes[link_id] += vote;
            }
        }
    }
    double sum_votes = accumulate(votes.begin(), votes.end(), 0.0, plus<double>());
    double max_votes = *max_element(votes.begin(), votes.end());
    return PDD(sum_votes, max_votes);
}

// Vote adjustment
void DoubleO7::LocalizeFailures(double min_start_time_ms, double max_finish_time_ms,
                                Hypothesis &localized_links, int nopenmp_threads){
    assert(data!=NULL);
    localized_links.clear();
    vector<Flow*> bad_flows; 
    for(Flow* flow: data->flows){
        if(flow->TracerouteFlow(max_finish_time_ms) > 0)
            bad_flows.push_back(flow);
    }
    int nlinks = data->inverse_links.size();
    vector<double> votes(nlinks, 0.0);
    if constexpr (VERBOSE){ 
        cout << "numflows "<< bad_flows.size();
    }
    auto [sum_votes, max_votes] = ComputeVotes(bad_flows, votes, localized_links,
                                          min_start_time_ms, max_finish_time_ms);
    if constexpr (VERBOSE){ 
        cout << " sumvote " << sum_votes << " maxvote " << max_votes
             << " numflows " << bad_flows.size() << endl;
    }
    while (max_votes >= fail_threshold * sum_votes and max_votes > 0){
        int faulty_link_id = distance(votes.begin(), max_element(votes.begin(), votes.end()));
        localized_links.insert(faulty_link_id);
        if(ADJUST_VOTES){
            tie(sum_votes, max_votes) = ComputeVotes(bad_flows, votes, localized_links,
                                                     min_start_time_ms, max_finish_time_ms);
        }
        else{
            votes[faulty_link_id] = 0; // Ensure it doesn't show up again
            sum_votes = accumulate(votes.begin(), votes.end(), 0.0, plus<double>());
            max_votes = *max_element(votes.begin(), votes.end());
        }
        if constexpr (VERBOSE){ 
            cout << " sumvote " << sum_votes << " maxvote " << max_votes
                 << " numflows " << bad_flows.size() << endl;
        }
    }
}
