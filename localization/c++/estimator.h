#ifndef __FAULT_LOCALIZE_ESTIMATOR__
#define __FAULT_LOCALIZE_ESTIMATOR__

#include "flow.h"
#include "utils.h"
#include "logdata.h"
#include <set>

class Estimator{
 public:
    Estimator(): data(NULL), flows_by_link_id(NULL) {}
     
    virtual void LocalizeFailures(double min_start_time_ms, double max_finish_time_ms, 
                                 Hypothesis &localized_links, int nopenmp_threads){
        assert(data != NULL);
    }

    virtual void SetLogData(LogData* data_, double max_finish_time_ms, int nopenmp_threads){
        data = data_;
    }

    void BinFlowsByLinkId(double max_finish_time_ms, int nopenmp_threads){
        auto start_bin_time = chrono::high_resolution_clock::now();
        if (flows_by_link_id != NULL) delete(flows_by_link_id);
        flows_by_link_id = data->GetFlowsByLinkId(max_finish_time_ms, nopenmp_threads);
        if constexpr (VERBOSE){
            cout << "Finished binning " << data->flows.size() << " flows by links in "
                 << GetTimeSinceMilliSeconds(start_bin_time) << " seconds" << endl;
        }
    }

    virtual Estimator* CreateObject() { return new Estimator(); }

    LogData* data;
    vector<vector<int> >* flows_by_link_id;
};

#endif
