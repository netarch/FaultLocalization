#ifndef __FAULT_LOCALIZE_UTILS__
#define __FAULT_LOCALIZE_UTILS__

#include "flow.h"
#include <map>
#include <unordered_map>
#include <set>
#include <mutex>
#include <shared_mutex>
#include <sparsehash/dense_hash_map>
using google::dense_hash_map;
using namespace std;

struct MemoizedPaths{
    dense_hash_map<int, vector<Path*> > paths_by_first_node;
    shared_mutex lock;
    MemoizedPaths() {
        paths_by_first_node.set_empty_key(-1000000);
    }
    Path* GetPath(vector<int>& vi_path){
        int first_node = (vi_path.size()>0? vi_path[0] : -1);
        lock.lock();
        vector<Path*> &paths = paths_by_first_node[first_node];
        Path* ret = NULL;
        for (Path* path: paths){
            if (*path == vi_path){
                ret = path;
                break;
            }
        }
        if (ret == NULL){
            ret = new Path(vi_path);
            paths.push_back(ret);
        }
        lock.unlock();
        return ret; 
    }
};

class LogData;

void GetDataFromLogFile(string filename, LogData* result);
void GetDataFromLogFileDistributed(string dirname, int nchunks, LogData* result, int nopenmp_threads);
PDD GetPrecisionRecall(Hypothesis& failed_links, Hypothesis& predicted_hypothesis);

#endif
