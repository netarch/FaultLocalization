#include <iostream>
#include <assert.h>
#include "utils.h"
#include "logdata.h"
#include <chrono>
#include <queue>

using namespace std;

struct LinkCtrs{
    int src, dst, ctr;
    LinkCtrs(int src_, int dst_, int ctr_): src(src_), dst(dst_), ctr(ctr_){}
    void hash_combine(long int& seed, int v){
        seed ^= std::hash<int>()(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    }
    long int hash(){
        long int seed = 0;
        hash_combine(seed, src);
        hash_combine(seed, dst);
        hash_combine(seed, ctr);
        return seed;
    }
    bool operator==(const LinkCtrs& rhs){
        return (
                (src == rhs.src) and 
                (dst == rhs.dst) and
                (ctr == rhs.ctr)
               );
    }
    bool operator<(const LinkCtrs& rhs){
        if (src != rhs.src) return (src < rhs.src);
        if (dst != rhs.dst) return (dst < rhs.dst);
        else return (ctr < rhs.ctr);
    }
};



void BuildPathsByLink(LogData &data, unordered_map<int, vector<LinkCtrs> > &result){
    for (auto &[src_dst, m_paths]: data.memoized_paths){
        vector<Path*> *paths;
        m_paths->GetAllPaths(&paths);
        unordered_map<int, int> link_ctrs;
        for (Path* path: *paths){
            for(int link_id: *path){
                link_ctrs[link_id] += 1;
            }
        }
        for(auto &[link_id, ctr]: link_ctrs){
            if (result.find(link_id) == result.end()) result[link_id] = vector<LinkCtrs>();
            result[link_id].push_back(LinkCtrs(src_dst.first, src_dst.second, ctr));
        }
    }
}


void HashLinks(LogData &data,
               unordered_map<int, vector<LinkCtrs> > &link_srcdst_ctrs,
               unordered_map<long int, vector<int> > &hash_links){
    int nlinks = data.inverse_links.size();
    for (int link_id=0; link_id<nlinks; link_id++){
        if (link_srcdst_ctrs.find(link_id) == link_srcdst_ctrs.end()) continue;
        long long int link_hash = 0.0;
        for (auto &link_ctrs: link_srcdst_ctrs[link_id]){
            link_hash = link_hash ^ link_ctrs.hash();
        }
        if (hash_links.find(link_hash) == hash_links.end()){
            hash_links[link_hash] = vector<int>();
        }
        hash_links[link_hash].push_back(link_id);
    }
}


void GetSameCtrSetEdges(unordered_map<int, vector<LinkCtrs> > &link_srcdst_ctrs,
                                  vector<int> &links,
                                  unordered_map<int, vector<int> > &edges){
    for (int link_id: links) edges[link_id] = vector<int>();
    for (int link_id1: links){
        for (int link_id2: links){
            if (link_id1 > link_id2){
                //!TODO: Make sure this is a copy by value, not by reference
                bool same = true;
                if (
                    (link_srcdst_ctrs[link_id1].size() != link_srcdst_ctrs[link_id2].size())
                    or (link_srcdst_ctrs[link_id1].size() == 0)
                   ) same = false;
                else{
                    vector<LinkCtrs> flows1 = link_srcdst_ctrs[link_id1];
                    vector<LinkCtrs> flows2 = link_srcdst_ctrs[link_id2];
                    //!TODO: possible optimization: sort all flow sets once at the beginning
                    sort(flows1.begin(), flows1.end());
                    sort(flows2.begin(), flows2.end());
                    for (int ii=0; ii<flows1.size(); ii++)
                        same = same && (flows1[ii] == flows2[ii]);
                }
                if (same){
                    edges[link_id1].push_back(link_id2);
                    edges[link_id2].push_back(link_id1);
                }
            }
        }
    }
}


// add connected_components to the result vector "connected_components"
// !TODO: would be cleaner with iterators
void GetConnectedComponents(vector<int> &links, unordered_map<int,
        vector<int> > &edges,
        vector<vector<int> > &connected_components){
    // now output connected components in this graph
    unordered_map<int, bool> visited;
    for (int link_id: links) visited[link_id] = false;
    for (int link_id: links){
        if (visited[link_id]) continue;
        queue<int> bfs_queue;
        vector<int> component;
        bfs_queue.push(link_id);
        visited[link_id] = true;
        while (!bfs_queue.empty()){
            int l = bfs_queue.front();
            bfs_queue.pop();
            component.push_back(l);
            for (int n: edges[l]){
                if (!visited[n]){
                    bfs_queue.push(n);
                    visited[n] = true;
                }
            }
        }
        connected_components.push_back(component);
    }
}


//!TODO: run Flock code by a formatter
//!TODO: check if long int addition additive?
void GetEquivalenceClasses(LogData &data, vector<vector<int> > &equivalence_classes){
    int nlinks = data.inverse_links.size();
    unordered_map<int, vector<LinkCtrs> > link_srcdst_ctrs;
    BuildPathsByLink(data, link_srcdst_ctrs);
    // use a hashing scheme to hash links based on flow_set
    unordered_map<long int, vector<int> > hash_links;
    HashLinks(data, link_srcdst_ctrs, hash_links);
    for (auto &[link_hash, links]: hash_links){
        // now check within this reduced class if the sets are the same
        // (1) create a graph with links as vertices
        // (2) add an edge between two links if they have the same counter sets
        // (3) output connected components in this graph
        unordered_map<int, vector<int> > edges;
        GetSameCtrSetEdges(link_srcdst_ctrs, links, edges);
        GetConnectedComponents(links, edges, equivalence_classes);
    }
}

int main(int argc, char *argv[]){
    VERBOSE = true;
    assert (argc == 6);
    string trace_file (argv[1]); 
    cout << "Running analysis on file "<<trace_file << endl;
    string topology_file (argv[2]);
    cout << "Reading topology from file " << topology_file << endl;
    double min_start_time_ms = atof(argv[3]) * 1000.0, max_finish_time_ms = atof(argv[4]) * 1000.0;
    int nopenmp_threads = atoi(argv[5]);
    cout << "Using " << nopenmp_threads << " openmp threads"<<endl;
    //int nchunks = 32;
    LogData data;
    //GetDataFromLogFile(trace_file, &data);
    // should go after declaring estimator
    GetDataFromLogFileParallel(trace_file, topology_file, &data, nopenmp_threads);
    //cout << "Num flows "<< data.flows.size() << endl;
    Hypothesis failed_links_set;
    data.GetFailedLinkIds(failed_links_set);

    auto start_time = chrono::high_resolution_clock::now();
    vector<vector<int> > equivalence_classes;
    GetEquivalenceClasses(data, equivalence_classes);

    int network_links = 0;
    int network_links_classes = 0;
    for (auto &v: equivalence_classes){
        Hypothesis link_ids(v.begin(), v.end()); 
        set<Link> links = data.IdsToLinks(link_ids);
        cout << "equivalence class: " << links << endl;
        if (v.size() > 1 or (v.size() == 1 and ! data.IsLinkDevice(*v.begin()))){
            network_links += links.size();
            network_links_classes++;
        }
    }

    cout << "Precision: " << network_links_classes/float(network_links) << endl;

    cout << "Finished in "<< GetTimeSinceSeconds(start_time) << " seconds" << endl;
    return 0;
}
