#include "bayesian_net.h"
#include <queue>

//!TODO: Change to smart pointers for easier GC
//!TODO: Change to smart pointers for easier GC
//!TODO: Change to smart pointers for easier GC
//!TODO: Change to smart pointers for easier GC
//!TODO: Change to smart pointers for easier GC
//!TODO: Change to smart pointers for easier GC
//!TODO: Change to smart pointers for easier GC
Hypothesis* BayesianNet::LocalizeFailures(LogFileData* data){
    data_cache = data;
    priority_queue<pair<double, Hypothesis*>,
                   vector<pair<double, Hypothesis*> >,
                   std::greater<double> > max_k_likelihoods;
   
    Hypothesis* no_failure_hypothesis = new Hypothesis();
    max_k_likelihoods.push(make_pair(0.0, no_failure_hypothesis));
    vector<Hypothesis*> prev_hypothesis_space; 
    prev_hypothesis_space.push_back(no_failure_hypothesis);
    int nfails = 1;
    bool repeat_nfails_1 = true;
    while (nfails <= MAX_FAILS){
        vector<Hypothesis*> hypothesis_space;
        for(Hypothesis* h: prev_hypothesis_space){
            assert (h != NULL);
            for (Link &link: *h){


            }


        }

    }

    Hypothesis* failed_links = new Hypothesis();
    while (max_k_likelihoods.size() > 0){
        pair<double, Hypothesis*> elt = max_k_likelihoods
        if (highest_likelihood - elt.first <= 1.0e-3){
            failed_links->insert(elt.second->begin(), elt.second->end());
        }
        if (VERBOSE){
            cout << "Likely candidate "<<elt.second<<" "<<elt.first << endl;
        }
    }
    return failed_links;
}
