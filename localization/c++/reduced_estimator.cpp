#include <iostream>
#include <assert.h>
#include "utils.h"
#include "bayesian_net.h"
#include <fstream>
#include <sstream>
#include <chrono>

using namespace std;


//Maybe take this mapping as an input file
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
            if (spine >= 90){
                //! MAJOR MAJOR HACK. 
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



int main(int argc, char *argv[]){
    assert (argc == 6);
    string flow_file (argv[1]); 
    double min_start_time_ms = atof(argv[2]) * 1000.0, max_finish_time_ms = atof(argv[3]) * 1000.0;
    int nopenmp_threads = atoi(argv[4]);
    string topology_file (argv[5]);
    cout << "Running analysis on file "<< flow_file << endl;
    cout << "Using topology file "<< topology_file <<endl;
    cout << "Using " << nopenmp_threads << " openmp threads"<<endl;
    //LogFileData* data = GetDataFromLogFile(filename);
    int nchunks = 32;
    LogFileData* data = GetDataFromLogFileDistributed(flow_file, nchunks, nchunks);
    BayesianNet estimator;
    if (estimator.USE_CONDITIONAL){
        data->FilterFlowsForConditional(max_finish_time_ms, nopenmp_threads);
    }
    // preprocessing for analysis on reduced graph
    unordered_map<Link, Link> to_reduced_graph;
    unordered_map<Link, vector<Link> > from_reduced_graph;
    ReducedGraphMappingLeafSpine(topology_file, to_reduced_graph, from_reduced_graph);
    cout << "Obtained reduced graph mappings" <<endl;
    cout << "Number of reduced nodes " << from_reduced_graph.size() << endl;
    LogFileData* reduced_data = new LogFileData();
    auto start_time = chrono::high_resolution_clock::now();
    data->GetReducedData(to_reduced_graph, *reduced_data, nopenmp_threads);
    cout << "Obtained reduced data for running analysis in " << chrono::duration_cast<chrono::milliseconds>(
            chrono::high_resolution_clock::now() - start_time).count()*1.0e-3 << " seconds" << endl;
    Hypothesis estimator_hypothesis;
    estimator.LocalizeFailures(reduced_data, min_start_time_ms, max_finish_time_ms,
                                        estimator_hypothesis, nopenmp_threads);
    Hypothesis failed_links_set;
    reduced_data->GetFailedLinkIds(failed_links_set);
    PDD precision_recall = GetPrecisionRecall(failed_links_set, estimator_hypothesis);
    cout << "Output Hypothesis: " << reduced_data->IdsToLinks(estimator_hypothesis) << " precsion_recall "
         <<precision_recall.first << " " << precision_recall.second<<endl;
    return 0;
}
