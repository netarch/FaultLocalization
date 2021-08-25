#include "net_bouncer.h"
#include "utils.h"
#include <assert.h>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <cmath>
#include <iostream>

using namespace std;

void NetBouncer::SetLogData(LogData* data_, double max_finish_time_ms, int nopenmp_threads){
    Estimator::SetLogData(data_, max_finish_time_ms, nopenmp_threads);
}

NetBouncer* NetBouncer::CreateObject() {
    NetBouncer* ret = new NetBouncer();
    vector<double> param {regularize_const, fail_threshold};
    ret->SetParams(param);
    return ret;
}
    
void NetBouncer::SetParams(vector<double>& params){
    assert(params.size() == 2);
    regularize_const = params[0];
    fail_threshold = params[1];
}

double NetBouncer::EstimatedPathSuccessRate(Flow *flow, vector<double>& success_prob){
    Path* path_taken = flow->GetPathTaken();
    double x = success_prob[flow->first_link_id] * success_prob[flow->last_link_id];
    for(int link_id: *path_taken){
        x *= success_prob[link_id];
    }
    return x;
}

void NetBouncer::DetectBadDevices(vector<int>& bad_devices, double min_start_time_ms,
                                  double max_finish_time_ms, int nopenmp_threads){
    BinFlowsByDevice(max_finish_time_ms, nopenmp_threads);
    bad_devices.clear();
    for (int device=0; device < data->GetMaxDevicePlus1(); device++){
        vector<int> &device_flows = (*flows_by_device)[device];
        bool good_device = false;
        for(int ff: device_flows){
            Flow *flow = data->flows[ff];
            assert(flow->TracerouteFlow(max_finish_time_ms));
            double y = 1.0 - flow->GetDropRate(max_finish_time_ms);
            if (abs(y - 1.0) < 1.0e-8){
                good_device = true;
                break;
            }
        }
        cout << "Device " << device <<", num_flows: " << device_flows.size() << " : " << good_device << endl; 
        if (!good_device) bad_devices.push_back(device);
    }
}

double NetBouncer::ComputeError(vector<Flow*>& active_flows, vector<double>& success_prob,
                                double min_start_time_ms, double max_finish_time_ms){
    double error = 0.0;
    int nlinks = data->inverse_links.size();
    for(int ff=0; ff < active_flows.size(); ff++){
        Flow *flow = active_flows[ff];
        Path* path_taken = flow->GetPathTaken();
        double x = EstimatedPathSuccessRate(flow, success_prob);
        double y = 1.0 - flow->GetDropRate(max_finish_time_ms);
        error += (y-x)*(y-x);
        //cout << error << " " << y << " " << x << " " << flow->GetPacketsSent(max_finish_time_ms)
        //     << " " << flow->GetPacketsLost(max_finish_time_ms) << " "
        //     << flow->GetDropRate(max_finish_time_ms) << endl;
    }
    for(int link_id=0; link_id<nlinks; link_id++){
        error += regularize_const * success_prob[link_id] * (1.0 - success_prob[link_id]);
    }
    return error;
}

double NetBouncer::ArgMinError(vector<double>& success_prob, int var_link_id,
                               double min_start_time_ms, double max_finish_time_ms){
    double s1=0.0, s2=0.0;
    vector<int> &link_id_flows = (*flows_by_link_id)[var_link_id];
    for(int ff: link_id_flows){
        Flow *flow = data->flows[ff];
        assert(flow->TracerouteFlow(max_finish_time_ms));
        Path *path_taken = flow->GetPathTaken();
        if (path_taken->find(var_link_id)!=path_taken->end() or flow->first_link_id == var_link_id
           or flow->last_link_id == var_link_id){
           double t = EstimatedPathSuccessRate(flow, success_prob)/success_prob[var_link_id];
           double y = 1.0 - flow->GetDropRate(max_finish_time_ms);
           s1 += t * y;
           s2 += t * t;
        }
    }
    return (2 * s1 - regularize_const)/(2 * (s2 - regularize_const));
}

void NetBouncer::LocalizeFailures(double min_start_time_ms, double max_finish_time_ms,
                                  Hypothesis &localized_links, int nopenmp_threads){
    BinFlowsByLinkId(max_finish_time_ms, nopenmp_threads);
    assert(data!=NULL);
    localized_links.clear();
    vector<Flow*> active_flows; 
    for(Flow* flow: data->flows){
        if((flow->IsFlowActive()) or PATH_KNOWN)
            active_flows.push_back(flow);
    }
    //vector<int> bad_devices;
    //DetectBadDevices(bad_devices, min_start_time_ms, max_finish_time_ms, nopenmp_threads);
    //cout << "Bad devices " << bad_devices << endl;
    int nlinks = data->inverse_links.size();
    vector<double> success_prob(nlinks, 0.0);
    vector<double> num_flows_through_link(nlinks, 0.0);

    //cout << "numflows " << active_flows.size() << " max_finish_time_ms " << max_finish_time_ms << endl;

    for(int ff=0; ff < active_flows.size(); ff++){
        Flow *flow = active_flows[ff];
        assert(flow->TracerouteFlow(max_finish_time_ms));
        Path *path_taken = flow->GetPathTaken();
        double flow_success_rate = 1.0 - flow->GetDropRate(max_finish_time_ms);
        double flow_drop_rate = flow->GetDropRate(max_finish_time_ms);
        //cout << ff << " " << flow->GetPacketsSent(max_finish_time_ms) << " " << flow_drop_rate << endl;
        if (flow->GetPacketsSent(max_finish_time_ms) == 0) continue;
        assert(flow->GetPacketsSent(max_finish_time_ms) > 0);
        double multiplier = 1; //flow->GetPacketsSent(max_finish_time_ms); //1
        success_prob[flow->first_link_id] += flow_success_rate * multiplier; 
        success_prob[flow->last_link_id] += flow_success_rate * multiplier; 
        num_flows_through_link[flow->first_link_id] += multiplier;
        num_flows_through_link[flow->last_link_id] += multiplier;
        for (int link_id: *path_taken){
            success_prob[link_id] += flow_success_rate * multiplier; 
            num_flows_through_link[link_id] += multiplier;
        }
    }
    for(int link_id=0; link_id<nlinks; link_id++){
        assert(num_flows_through_link[link_id] > 0);
        success_prob[link_id] /= num_flows_through_link[link_id];
    }
    double error = ComputeError(active_flows, success_prob, min_start_time_ms, max_finish_time_ms);
    for (int it=0; it<MAX_ITERATIONS; it++){
        auto start_iter_time = chrono::high_resolution_clock::now();
        for(int link_id=0; link_id < nlinks; link_id++){
            success_prob[link_id] = max(0.0, min(1.0, ArgMinError(success_prob, link_id,
                                                        min_start_time_ms, max_finish_time_ms)));
        }
        double new_error = ComputeError(active_flows, success_prob, min_start_time_ms, max_finish_time_ms);
        if constexpr (VERBOSE){
            cout << "Iteration " << it << " error " << new_error << " finished in "
                 << GetTimeSinceSeconds(start_iter_time) * 1.0e-3 << endl;
        }
        if (abs(new_error - error) < 1.0e-9) break;
        error = new_error;
    }

    localized_links.clear();
    for(int link_id=0; link_id<nlinks; link_id++){
        if (VERBOSE and (1.0 - success_prob[link_id] >= fail_threshold/2)){
            //cout << "Suspicious link "<< data->inverse_links[link_id] << " "
            //     << 1.0 - success_prob[link_id] << " " << fail_threshold << endl;
        }
        if (1.0 - success_prob[link_id] >= fail_threshold){
            localized_links.insert(link_id);
        }
    }
}
