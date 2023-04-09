#include "bayesian_net.h"
#include "utils.h"
#include <algorithm>
#include <assert.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;

// Take reduced graph mapping as an input file
void ReducedGraphMappingFromFile(
    string reduced_map_file, unordered_map<Link, Link> &to_reduced_graph,
    unordered_map<Link, vector<Link>> &from_reduced_graph) {
    ifstream mfile(reduced_map_file);
    string line;
    while (getline(mfile, line)) {
        replace(line.begin(), line.end(), '(', ' ');
        replace(line.begin(), line.end(), ')', ' ');
        replace(line.begin(), line.end(), ',', ' ');
        istringstream line_stream(line);
        int l1, l2, r1, r2;
        line_stream >> l1 >> l2 >> r1 >> r2;
        Link link(l1, l2), reduced_link(r1, r2);
        // cout << "Reduced link " << link << " " << reduced_link << endl;
        to_reduced_graph[link] = reduced_link;
        from_reduced_graph[reduced_link].push_back(link);
    }
}

//! TODO: Deprecated
void ReducedGraphMappingLeafSpine(
    string topology_file, unordered_map<Link, Link> &to_reduced_graph,
    unordered_map<Link, vector<Link>> &from_reduced_graph) {
    ifstream tfile(topology_file);
    string line;
    // Collapse the spine layer into one switch
    int spine_sw_id = 0;
    while (getline(tfile, line)) {
        istringstream line_stream(line);
        if (line.find("->") == string::npos) {
            int spine, leaf;
            line_stream >> spine >> leaf;
            int nspines = 30;
            if (spine >= nspines) {
                //! MAJOR MAJOR HACK FOR A SPECIFIC LEAFSPINE TOPOLOGY
                // swap leaf and spine
                int temp = spine;
                spine = leaf;
                leaf = temp;
            }

            Link downlink(spine, leaf), reduced_downlink(spine_sw_id, leaf);
            to_reduced_graph[downlink] = reduced_downlink;
            from_reduced_graph[reduced_downlink].push_back(downlink);

            Link uplink(leaf, spine), reduced_uplink(leaf, spine_sw_id);
            to_reduced_graph[uplink] = reduced_uplink;
            from_reduced_graph[reduced_uplink].push_back(uplink);
        } else {
            //! TODO: (svr --> sw) ignore for now
        }
    }
}

void SetInputForReduced(BayesianNet &estimator, LogData *data,
                        LogData *reduced_data, string reduced_map_file,
                        double max_finish_time_ms, int nopenmp_threads) {
    // Preprocess to obtain reduced graph
    // !TODO prevent memory leak
    unordered_map<Link, Link> *to_reduced_graph =
        new unordered_map<Link, Link>();
    unordered_map<Link, vector<Link>> *from_reduced_graph =
        new unordered_map<Link, vector<Link>>();
    ReducedGraphMappingFromFile(reduced_map_file, *to_reduced_graph,
                                *from_reduced_graph);
    unordered_map<int, int> *num_reduced_links_map =
        new unordered_map<int, int>();

    // store of data for reduced graph in reduced_data
    auto start_time = chrono::high_resolution_clock::now();
    data->GetReducedData(*to_reduced_graph, *reduced_data, nopenmp_threads);
    GetNumReducedLinksMap(*to_reduced_graph, *data, *reduced_data,
                          *num_reduced_links_map);
    if (VERBOSE) {
        cout << "Number of reduced nodes " << from_reduced_graph->size()
             << " Obtained reduced data for running analysis in "
             << GetTimeSinceSeconds(start_time) << " seconds" << endl;
    }
    estimator.SetReducedAnalysis(true);
    estimator.SetLogData(reduced_data, max_finish_time_ms, nopenmp_threads);
    estimator.SetNumReducedLinksMap(num_reduced_links_map);
}

//! TODO: take a vector of (start_time_ms, end_time_ms, remove_link)
LogData* RemoveLinksPassiveOnly(BayesianNet &estimator, LogData *data,
                                double start_time_ms, double end_time_ms,
                                double max_finish_time_ms, int remove_link_id,
                                int nopenmp_threads) {

    LogData *data_after_remove = new LogData();
    data_after_remove->failed_links = data->failed_links;
    data_after_remove->failed_devices = data->failed_devices;
    data_after_remove->hosts_to_racks = data->hosts_to_racks;
    data_after_remove->inverse_links = data->inverse_links;
    data_after_remove->links_to_ids = data->links_to_ids;

    for (Flow *f : data->flows) {
        Path *path_taken = f->GetPathTaken();
        double f_end_time_ms = f->GetEndTime();
        if ((f->start_time_ms > start_time_ms and
             f_end_time_ms < end_time_ms) and
            path_taken->find(remove_link_id) == path_taken->end()) {
            data_after_remove->flows.push_back(f);
        }
    }

    for (auto &[src_dst, mpaths] : data->memoized_paths) {
        MemoizedPaths *mpaths_new = new MemoizedPaths();
        for (int ii = 0; ii < mpaths->paths.size(); ii++) {
            Path *path = mpaths->paths[ii];
            if (path->find(remove_link_id) == path->end()) {
                mpaths_new->paths.push_back(path);
            }
        }
        assert(mpaths_new->paths.size() > 0);
        data_after_remove->memoized_paths[src_dst] = mpaths_new;
    }
    return data_after_remove;
}
