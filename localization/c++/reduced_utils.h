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
                                 unordered_map<Link, vector<Link> >& from_reduced_graph);
//!TODO: Deprecated
void ReducedGraphMappingLeafSpine(string topology_file, unordered_map<Link, Link> &to_reduced_graph,
                                  unordered_map<Link, vector<Link> >& from_reduced_graph);

void SetInputForReduced(BayesianNet &estimator, LogData* data, LogData *reduced_data,
                        string reduced_map_file, double max_finish_time_ms, int nopenmp_threads);