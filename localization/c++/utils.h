#ifndef __FAULT_LOCALIZE_UTILS__
#define __FAULT_LOCALIZE_UTILS__

#include "flow.h"
#include <map>
#include <unordered_map>
#include <set>
#include <mutex>
#include <utility>
#include <shared_mutex>
#include <algorithm>
#include <locale>
#include <string>
#include <sparsehash/dense_hash_map>
using google::dense_hash_map;
using namespace std;

inline constexpr bool PARALLEL_IO=true;

struct MemoizedPaths{
    dense_hash_map<int, vector<Path*> > paths_by_first_node;
    shared_mutex lock;
    MemoizedPaths() {
        paths_by_first_node.set_empty_key(-1000000);
    }
    Path* GetPath(vector<int>& vi_path){
        int first_node = (vi_path.size()>0? vi_path[0] : -1);
        if constexpr (PARALLEL_IO) lock.lock();
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
        if constexpr (PARALLEL_IO) lock.unlock();
        return ret; 
    }
    void GetAllPaths(vector<Path*> &result){
        for (auto &[node, paths]: paths_by_first_node){
            for(Path *path: paths)
                result.push_back(path);
        }
    }
};

class LogData;
void GetDataFromLogFile(string filename, LogData* result);
void GetDataFromLogFile1(string filename, LogData* result);
void GetDataFromLogFileDistributed(string dirname, int nchunks, LogData* result, int nopenmp_threads);

inline bool HypothesisIntersectsPath(Hypothesis *hypothesis, Path *path){
    bool link_in_path = false;
    for (int link_id: *path){
        link_in_path = (link_in_path or (hypothesis->count(link_id) > 0));
    }
    return link_in_path;
}

inline int GetTokenLength(char *str){
    int ind = 0;
    while(str[ind]==' ') ind++; //Trim initial whitespace
    while (str[ind]!='\0' and str[ind]!=' ') ind++;
    return ind;
}

inline pair<char*, bool> to_int(char *s, int length, int &result){
    //cout << "converting " << string(s, length) << " ";
    char *end = s + length;
    if ( s == NULL || length==0 ) return pair<char*, bool>(s, false);
    while(*s == ' ') ++s;
    bool negate = (s[0] == '-');
    if (*s == '+' || *s == '-') ++s;
    if (s == end) return pair<char*, bool>(s, false);

    result = 0;
    while(s!=end){
        if (*s >= '0' && *s <= '9'){
            result = result * 10  + (*s - '0');
        }
        else return pair<char*, bool>(s, false);
        ++s;
    }
    if (negate) result = -result;
    //cout << result << endl;
    return pair<char*, bool>(s, true);
} 

inline bool GetFirstInt(char* &str, int &result){
    int token_length = GetTokenLength(str);
    //auto [p, ec] = from_chars(str, str + token_length, result);
    auto [p, success] = to_int(str, token_length, result);
    str = p;
    return success;
}
/*
inline bool GetFirstInt(char* &str, int &result){
    int token_length = GetTokenLength(str);
    char c = str[token_length]; str[token_length] = '\0'; //temporary sentinel value
    bool all_white_space = all_of(str, str+token_length, [](char c){return isspace(c);});
    //auto [p, ec] = from_chars(str, str + token_length, result);
    //str = p;
    if (!all_white_space) result = atoi(str);
    //cout << str << " : " << result <<endl;
    str[token_length] = c;
    str += token_length;
    return !all_white_space;
}
*/

inline bool GetFirstDouble(char* &str, double &result){
    int token_length = GetTokenLength(str);
    char c = str[token_length]; str[token_length] = '\0'; //temporary sentinel value
    bool all_white_space = all_of(str, str+token_length, [](char c){return isspace(c);});
    //auto [p, ec] = from_chars(str, str + token_length, result);
    //str = p;
    if (!all_white_space) result = atof(str);
    //cout << str << " : " << result <<endl;
    str[token_length] = c;
    str += token_length;
    return !all_white_space;
}

inline bool GetString(char* &str, string &result){
    int token_length = GetTokenLength(str);
    result.clear();
    result.insert(0, str, token_length);
    str = str + token_length;
    return true;
}

PDD GetPrecisionRecall(Hypothesis& failed_links, Hypothesis& predicted_hypothesis);

#endif
