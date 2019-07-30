#include "utils.h"
#include <assert.h>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>


inline bool string_starts_with(const string &a, const string &b) {
    return (b.length() <= a.length() && equal(b.begin(), b.end(), a.begin()));
}

LogFileData* GetDataFromLogFile(string filename){ 
    auto start_time = chrono::high_resolution_clock::now();
    LogFileData* data = new LogFileData();
    ifstream infile(filename);
    string line, op;
    Flow *flow = NULL;
    int curr_link_index = 0;
    int nlines = 0;
    while (getline(infile, line)){
        istringstream line_stream (line);
        line_stream >> op;
        nlines += 1;
        if (op == "Failing_link"){
            int src, dest;
            float failparam;
            line_stream >> src >> dest >> failparam;
            data->failed_links[Link(src, dest)] = failparam;
            if (VERBOSE){
                cout<< "Failed link "<<src<<" "<<dest<<" "<<failparam<<endl; 
            }
        }
        else if (op == "Flowid="){
            // Log the previous flow
            if (flow != NULL and flow->paths.size() > 0){
                assert (flow->GetPathTaken() and flow->GetReversePathTaken());
                if (flow->GetLatestPacketsSent() > 0 and !flow->DiscardFlow() and flow->paths.size() > 0){
                    //flow->PrintInfo();
                    data->flows.push_back(flow);
                }
            }
            int src, dest, nbytes;
            double start_time_ms;
            line_stream >> src >> dest >> nbytes >> start_time_ms;
            flow = new Flow(src, "", 0, dest, "", 0, nbytes, start_time_ms);
        }
        else if (op == "Snapshot"){
            double snapshot_time_ms;
            int packets_sent, packets_lost, packets_randomly_lost;
            line_stream >> snapshot_time_ms >> packets_sent >> packets_lost >> packets_randomly_lost;
            assert (flow != NULL);
            flow->AddSnapshot(snapshot_time_ms, packets_sent, packets_lost, packets_randomly_lost);
        }
        else if (string_starts_with(op, "flowpath_reverse")){
            Path* path = new Path();
            int node;
            while (line_stream >> node){
                path->push_back(node);
            }
            if (path->size() > 0){
                assert (flow != NULL);
                flow->AddReversePath(path, (op.find("taken") != string::npos));
            }
            for (int i=1; i<path->size(); i++){
                Link link(path->at(i-1), path->at(i));
                if (data->links_to_indices.find(link) == data->links_to_indices.end()){
                    data->links_to_indices[link] = curr_link_index++;
                    data->inverse_links.push_back(link);
                    assert (data->inverse_links.size() == curr_link_index);
                }
            }
        }
        else if (string_starts_with(op, "flowpath")){
            Path* path = new Path();
            int node;
            while (line_stream >> node){
                path->push_back(node);
            }
            if (path->size() > 0){
                assert (flow != NULL);
                flow->AddPath(path, (op.find("taken") != string::npos));
            }
            for (int i=1; i<path->size(); i++){
                Link link(path->at(i-1), path->at(i));
                if (data->links_to_indices.find(link) == data->links_to_indices.end()){
                    data->links_to_indices[link] = curr_link_index++;
                    data->inverse_links.push_back(link);
                    assert (data->inverse_links.size() == curr_link_index);
                }
            }
        }
        else if (op == "link_statistics"){
            assert (false);
        }
    }

    // Log the last flow
    if (flow != NULL and flow->paths.size() > 0){
        assert (flow->GetPathTaken() and flow->GetReversePathTaken());
        if (flow->GetLatestPacketsSent() > 0 and !flow->DiscardFlow() and flow->paths.size() > 0){
            //flow->PrintInfo();
            data->flows.push_back(flow);
        }
    }
    if (VERBOSE){
        cout<<"Read log file in "<<chrono::duration_cast<chrono::seconds>(
            chrono::high_resolution_clock::now() - start_time).count() << " seconds, numlines " << nlines << endl;
    }
    return data;
}

unordered_map<Link, vector<int> >* LogFileData::GetForwardFlowsByLink(double max_finish_time_ms){
    if (forward_flows_by_link != NULL) delete(forward_flows_by_link);
    forward_flows_by_link = new unordered_map<Link, vector<int> >();
    for (int ff=0; ff<flows.size(); ff++){
        auto flow_paths = flows[ff]->GetPaths(max_finish_time_ms);
        for (Path* path: *flow_paths){
            for (int i=1; i<path->size(); i++){
                Link link(path->at(i-1), path->at(i));
                (*forward_flows_by_link)[link].push_back(ff);
            }
        }
    }
    return forward_flows_by_link;
}

unordered_map<Link, vector<int> >* LogFileData::GetReverseFlowsByLink(double max_finish_time_ms){
    if (reverse_flows_by_link != NULL) delete(reverse_flows_by_link);
    reverse_flows_by_link = new unordered_map<Link, vector<int> >();
    for (int ff=0; ff<flows.size(); ff++){
        auto flow_reverse_paths = flows[ff]->GetReversePaths(max_finish_time_ms);
        for (Path* path: *flow_reverse_paths){
            for (int i=1; i<path->size(); i++){
                Link link(path->at(i-1), path->at(i));
                (*reverse_flows_by_link)[link].push_back(ff);
            }
        }
    }
    return reverse_flows_by_link;
}

unordered_map<Link, vector<int> >* LogFileData::GetFlowsByLink(double max_finish_time_ms){
    if (flows_by_link != NULL) delete(flows_by_link);
    GetForwardFlowsByLink(max_finish_time_ms);
    GetReverseFlowsByLink(max_finish_time_ms);
    if (!CONSIDER_REVERSE_PATH) flows_by_link = forward_flows_by_link;
    else{
        flows_by_link = new unordered_map<Link, vector<int> >();
        for (Link link: inverse_links){
            set_union(forward_flows_by_link->at(link).begin(), forward_flows_by_link->at(link).end(),
                    reverse_flows_by_link->at(link).begin(), reverse_flows_by_link->at(link).end(),
                    back_inserter(flows_by_link->at(link)));
            //!TODO: Check if flows_by_link actually gets populated
        }
    }
    return flows_by_link;
}

void LogFileData::GetFailedLinksSet(Hypothesis &failed_links_set){
    for (auto &it: failed_links){
        failed_links_set.insert(it.first);
    } 
}

PDD GetPrecisionRecall(Hypothesis& failed_links, Hypothesis& predicted_hypothesis){
    vector<Link> correctly_predicted;
    set_intersection(failed_links.begin(), failed_links.end(),
                     predicted_hypothesis.begin(), predicted_hypothesis.end(),
                     back_inserter(correctly_predicted));
    double precision = 1.0, recall = 1.0;
    if (predicted_hypothesis.size() > 0) {
        precision = ((double) correctly_predicted.size())/predicted_hypothesis.size();
    }
    if (failed_links.size() > 0){
        recall = ((double) correctly_predicted.size())/failed_links.size();
    }
    return PDD(precision, recall);
}

