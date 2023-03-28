#ifndef __FAULT_LOCALIZE_ESTIMATOR__
#define __FAULT_LOCALIZE_ESTIMATOR__

#include "flow.h"
#include "logdata.h"
#include "utils.h"
#include <set>

class Estimator {
  public:
    Estimator() : data(NULL), flows_by_link_id(NULL), flows_by_device(NULL) {}

    virtual void LocalizeFailures(double min_start_time_ms,
                                  double max_finish_time_ms,
                                  Hypothesis &localized_links,
                                  int nopenmp_threads) {
        assert(data != NULL);
    }

    virtual void SetLogData(LogData *data_, double max_finish_time_ms,
                            int nopenmp_threads) {
        data = data_;
    }

    virtual void SetParams(vector<double> &param) { assert(false); }

    void BinFlowsByLinkId(double max_finish_time_ms, int nopenmp_threads) {
        auto start_bin_time = chrono::high_resolution_clock::now();
        if (flows_by_link_id != NULL)
            delete (flows_by_link_id);
        flows_by_link_id =
            data->GetFlowsByLinkId(max_finish_time_ms, nopenmp_threads);
        if (VERBOSE) {
            int num_flows =
                count_if(data->flows.begin(), data->flows.end(),
                         [max_finish_time_ms](Flow *flow) {
                             return flow->AnySnapshotBefore(max_finish_time_ms);
                         });
            cout << "Finished binning " << num_flows << " flows by links in "
                 << GetTimeSinceSeconds(start_bin_time) << " seconds" << endl;
        }
    }

    void BinFlowsByDevice(double max_finish_time_ms, int nopenmp_threads) {
        auto start_bin_time = chrono::high_resolution_clock::now();
        if (flows_by_device != NULL)
            delete (flows_by_device);
        flows_by_device =
            data->GetFlowsByDevice(max_finish_time_ms, nopenmp_threads);
        if (VERBOSE) {
            int num_flows =
                count_if(data->flows.begin(), data->flows.end(),
                         [max_finish_time_ms](Flow *flow) {
                             return flow->AnySnapshotBefore(max_finish_time_ms);
                         });
            cout << "Finished binning " << num_flows << " flows by devices in "
                 << GetTimeSinceSeconds(start_bin_time) << " seconds" << endl;
        }
    }

    virtual Estimator *CreateObject() { return new Estimator(); }

    LogData *data;
    vector<vector<int>> *flows_by_link_id;
    vector<vector<int>> *flows_by_device;
};

#endif
