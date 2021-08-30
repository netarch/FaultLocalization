#include <iostream>
#include <assert.h>
#include "utils.h"
#include "bayesian_net.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>

using namespace std;

//Take reduced graph mapping as an input file
void ReducedGraphMappingFromFile(string reduced_map_file, unordered_map<Link, Link> &to_reduced_graph,
                                 unordered_map<Link, vector<Link> >& from_reduced_graph){
    ifstream mfile(reduced_map_file);
    string line;
    while (getline(mfile, line)){
        replace(line.begin(), line.end(), '(', ' ');
        replace(line.begin(), line.end(), ')', ' ');
        replace(line.begin(), line.end(), ',', ' ');
        istringstream line_stream (line);
        int l1, l2, r1, r2;
        line_stream >> l1 >> l2 >> r1 >> r2;
        Link link(l1, l2), reduced_link(r1, r2);
        //cout << "Reduced link " << link << " " << reduced_link << endl;
        to_reduced_graph[link] = reduced_link;
        from_reduced_graph[reduced_link].push_back(link);
    }
}

//!TODO: Deprecated
void ReducedGraphMappingLeafSpine(string topology_file, unordered_map<Link, Link> &to_reduced_graph,
                                  unordered_map<Link, vector<Link> >& from_reduced_graph){
    ifstream tfile(topology_file);
    string line;
    //Collapse the spine layer into one switch
    int spine_sw_id = 0;
    while (getline(tfile, line)){
        istringstream line_stream (line);
        if (line.find("->") == string::npos){
            int spine, leaf;
            line_stream >> spine >> leaf;
            int nspines = 30;
            if (spine >= nspines){
                //! MAJOR MAJOR HACK FOR A SPECIFIC LEAFSPINE TOPOLOGY
                //swap leaf and spine
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
        }
        else{
            //!TODO: (svr --> sw) ignore for now
        }
    }
}

void SetInputForReduced(BayesianNet &estimator, LogData* data, LogData *reduced_data,
                        string reduced_map_file, double max_finish_time_ms, int nopenmp_threads){
    // Preprocess to obtain reduced graph
    // !TODO prevent memory leak
    unordered_map<Link, Link> *to_reduced_graph = new unordered_map<Link, Link>();
    unordered_map<Link, vector<Link> > *from_reduced_graph = new unordered_map<Link, vector<Link> >();
    ReducedGraphMappingFromFile(reduced_map_file, *to_reduced_graph, *from_reduced_graph);
    unordered_map<int, int> *num_reduced_links_map = new unordered_map<int, int>();
    
    // store of data for reduced graph in reduced_data
    auto start_time = chrono::high_resolution_clock::now();
    data->GetReducedData(*to_reduced_graph, *reduced_data, nopenmp_threads);
    GetNumReducedLinksMap(*to_reduced_graph, *data, *reduced_data, *num_reduced_links_map);
    if (VERBOSE) {
        cout << "Number of reduced nodes " << from_reduced_graph->size()
             << " Obtained reduced data for running analysis in "
             << GetTimeSinceSeconds(start_time) << " seconds" << endl;
    }
    estimator.SetReducedAnalysis(true);
    estimator.SetLogData(reduced_data, max_finish_time_ms, nopenmp_threads);
    estimator.SetNumReducedLinksMap(num_reduced_links_map);
}
