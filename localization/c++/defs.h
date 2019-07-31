#ifndef __FAULT_LOCALIZE_DEFS__
#define __FAULT_LOCALIZE_DEFS__

#include <vector>
#include <set>
#include <unordered_set>

using namespace std;

typedef pair<int, int> PII;
typedef pair<double, double> PDD;
typedef vector<int> Path;
typedef PII Link;
//!TODO: run speed tests b/w set and unordered_set
typedef set<Link, less<Link>, vector<Link> > Hypothesis;

namespace std {
    template <>
    struct hash<PII>{
        std::size_t operator()(const PII& pii) const {
            return hash<int>()(pii.first) ^ hash<int>()(pii.second);
        }
    };
}

struct HypothesisPointerCompare{
    explicit HypothesisPointerCompare(Hypothesis *h_):h(h_) {}
    inline bool operator()(const Hypothesis* h2) const {
        return *h == *h2;
    }
private:
    Hypothesis *h;
};


// C++ template to print pair<> 
// class by using template 
template <typename T, typename S> 
ostream& operator<<(ostream& os, const pair<T, S>& v) { 
    os << "("; 
    os << v.first << ", " 
        << v.second << ")"; 
    return os; 
}

// C++ template to print vector container elements 
template <typename T> 
ostream& operator<<(ostream& os, const vector<T>& v) { 
    os << "["; 
    for (int i = 0; i < v.size(); ++i) { 
        os << v[i]; 
        if (i != v.size() - 1) 
            os << ", "; 
    }
    os << "]"; 
    return os; 
} 

// C++ template to print set container elements 
template <typename T> 
ostream& operator<<(ostream& os, const set<T>& v) { 
    os << "["; 
    for (auto it : v) { 
        os << it; 
        if (it != *v.rbegin()) 
            os << ", "; 
    } 
    os << "]"; 
    return os; 
} 

// C++ template to print unordered_set container elements 
template <typename T> 
ostream& operator<<(ostream& os, const unordered_set<T>& v) { 
    os << "["; 
    bool first = true; 
    for (auto it : v) { 
        if (! first) os <<", ";
        os << it; 
        first = false;
    } 
    os << "]"; 
    return os; 
} 

const bool PATH_KNOWN=false;
const bool CONSIDER_REVERSE_PATH=false;
const bool VERBOSE=true;

#endif
