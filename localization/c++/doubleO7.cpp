#include "doubleO7.h"
#include "utils.h"
#include <assert.h>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <cmath>
#include <iostream>

using namespace std;

DoubleO7::DoubleO7(): Estimator() {
    CONSIDER_DEVICE_LINK = false;
    TRACEROUTE_BAD_FLOWS = true;
    INPUT_FLOW_TYPE = PROBLEMATIC_FLOWS;
}

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
            //cout << " path_taken " << *path_taken << " " << flow_vote << endl;
            if (FILTER_NOISY_DROPS){
                flow_vote = (int) (flow->LabelWeightsFunc(max_finish_time_ms).second > 1);
            }
            else{
                assert(flow_vote == 1 or flow->IsFlowActive());
            }
            int total_path_length = path_taken->size() + 2;
            double vote = ((double)flow_vote) / total_path_length;
            if constexpr(USE_BUGGY_007) vote = ((double)flow_vote) / (total_path_length - 1);
            votes[flow->first_link_id] += vote;
            votes[flow->last_link_id] += vote;
            if constexpr (USE_BUGGY_007) votes[flow->last_link_id] -= vote;
            for (int link_id: *path_taken){
                votes[link_id] += vote;
            }
        }
    }
    double sum_votes = accumulate(votes.begin(), votes.end(), 0.0, plus<double>());
    double max_votes = *max_element(votes.begin(), votes.end());
    return PDD(sum_votes, max_votes);
}

void DoubleO7::LocalizeFailures(double min_start_time_ms, double max_finish_time_ms,
                                Hypothesis &localized_components, int nopenmp_threads){
    if (DEVICE_ANALYSIS)
        LocalizeDeviceFailures(min_start_time_ms, max_finish_time_ms,
                               localized_components, nopenmp_threads);
    else
        LocalizeLinkFailures(min_start_time_ms, max_finish_time_ms,
                             localized_components, nopenmp_threads);
}
// Vote adjustment
void DoubleO7::LocalizeLinkFailures(double min_start_time_ms, double max_finish_time_ms,
                                    Hypothesis &localized_links, int nopenmp_threads){
    assert (!CONSIDER_DEVICE_LINK and TRACEROUTE_BAD_FLOWS);
    localized_links.clear();
    vector<Flow*> bad_flows; 
    for(Flow* flow: data->flows){
        if(flow->TracerouteFlow(max_finish_time_ms))
            bad_flows.push_back(flow);
    }
    int nlinks = data->inverse_links.size();
    vector<double> votes(nlinks, 0.0);
    if (VERBOSE){ 
        cout << "numflows "<< bad_flows.size();
    }
    auto [sum_votes, max_votes] = ComputeVotes(bad_flows, votes, localized_links,
                                          min_start_time_ms, max_finish_time_ms);
    if (VERBOSE){ 
        cout << " sumvote " << sum_votes << " maxvote " << max_votes
             << " numflows " << bad_flows.size() << endl;
    }
    /* print scores for verification
    for (int link_id=0; link_id<nlinks; link_id++){
        Link link = data->inverse_links[link_id];
        cout << (link.first > 10000? link.first - 10000 : link.first) << " "
             << (link.second > 10000? link.second - 10000: link.second) << " " << votes[link_id] << endl;
    }
    return;
    */
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
        if (VERBOSE){ 
            cout << " sumvote " << sum_votes << " maxvote " << max_votes
                 << " numflows " << bad_flows.size() << endl;
        }
    }
}

PDD DoubleO7::ComputeVotesDevice(vector<Flow*>& bad_flows, vector<double>& votes,
                                 Hypothesis &problematic_devices, double min_start_time_ms,
                                 double max_finish_time_ms){
    fill(votes.begin(), votes.end(), 0.0);
    Path device_path;
    for(int ff=0; ff < bad_flows.size(); ff++){
        Flow *flow = bad_flows[ff];
        assert(flow->TracerouteFlow(max_finish_time_ms));
        data->GetDeviceLevelPath(flow, *flow->GetPathTaken(), device_path);
        if (!HypothesisIntersectsPath(&problematic_devices, &device_path)){
            int flow_vote = (int) (flow->LabelWeightsFunc(max_finish_time_ms).second > 0);
            //cout << " path_taken " << *path_taken << " " << flow_vote << endl;
            if (FILTER_NOISY_DROPS){
                flow_vote = (int) (flow->LabelWeightsFunc(max_finish_time_ms).second > 1);
            }
            else{
                assert(flow_vote == 1 or flow->IsFlowActive());
            }
            int total_path_length = device_path.size();
            double vote = ((double)flow_vote) / total_path_length;
            for (int device: device_path){
                votes[device] += vote;
            }
        }
    }
    double sum_votes = accumulate(votes.begin(), votes.end(), 0.0, plus<double>());
    double max_votes = *max_element(votes.begin(), votes.end());
    return PDD(sum_votes, max_votes);
}

void DoubleO7::LocalizeDeviceFailures(double min_start_time_ms, double max_finish_time_ms,
                                      Hypothesis &localized_device_links, int nopenmp_threads){
    assert (!CONSIDER_DEVICE_LINK and TRACEROUTE_BAD_FLOWS);
    vector<Flow*> bad_flows; 
    for(Flow* flow: data->flows){
        if(flow->TracerouteFlow(max_finish_time_ms))
            bad_flows.push_back(flow);
    }
    int ndevices = data->GetMaxDevicePlus1();
    vector<double> votes(ndevices, 0.0);
    if (VERBOSE) cout << "numflows "<< bad_flows.size();
    Hypothesis localized_devices;
    auto [sum_votes, max_votes] = ComputeVotesDevice(bad_flows, votes, localized_devices,
                                                     min_start_time_ms, max_finish_time_ms);
    if (VERBOSE){ 
        cout << " sumvote " << sum_votes << " maxvote " << max_votes
             << " numflows " << bad_flows.size() << endl;
    }
    while (max_votes >= fail_threshold * sum_votes and max_votes > 0){
        int faulty_device = distance(votes.begin(), max_element(votes.begin(), votes.end()));
        localized_devices.insert(faulty_device);
        if(ADJUST_VOTES){
            tie(sum_votes, max_votes) = ComputeVotesDevice(bad_flows, votes, localized_devices,
                                                     min_start_time_ms, max_finish_time_ms);
        }
        else{
            votes[faulty_device] = 0; // Ensure it doesn't show up again
            sum_votes = accumulate(votes.begin(), votes.end(), 0.0, plus<double>());
            max_votes = *max_element(votes.begin(), votes.end());
        }
        if (VERBOSE){ 
            cout << " sumvote " << sum_votes << " maxvote " << max_votes
                 << " numflows " << bad_flows.size() << endl;
        }
    }
    localized_device_links.clear();
    for (int device: localized_devices)
        localized_device_links.insert(data->GetLinkIdUnsafe(Link(device, device)));
}
