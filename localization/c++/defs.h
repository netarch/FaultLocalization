#ifndef __FAULT_LOCALIZE_DEFS__
#define __FAULT_LOCALIZE_DEFS__

#include <vector>
#include <set>

using namespace std;

typedef pair<int, int> PII;
typedef pair<double, double> PDD;
typedef vector<int> Path;
typedef PII Link;
typedef set<Link> Hypothesis;

namespace std {
    template <>
    struct hash<PII>{
        std::size_t operator()(const PII& pii) const {
            return hash<int>()(pii.first) ^ hash<int>()(pii.second);
        }
    };
}

const bool PATH_KNOWN=false;
const bool CONSIDER_REVERSE_PATH=false;
const bool VERBOSE=true;

#endif
