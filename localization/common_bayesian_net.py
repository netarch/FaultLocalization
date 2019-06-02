import sys
from heapq import heappush, heappop
from multiprocessing import Process, Queue
import math
import time
import joblib
from joblib import Parallel, delayed
import random
import numpy as np
from scipy.optimize import minimize
import utils
from utils import *
from plot_utils import *

def fn(naffected, npaths, naffected_r, npaths_r, p1, p2):
    #end_to_end: e2e
    e2e_paths = npaths * npaths_r
    e2e_correct_paths = (npaths - naffected) * (npaths_r - naffected_r)
    e2e_failed_paths = e2e_paths - e2e_correct_paths
    return (e2e_failed_paths * (1.0 - p1)/e2e_paths + e2e_correct_paths * p2/e2e_paths)

#math.log((1.0 - 1.0/k) + (1.0-p1)/(p2*k))
def bnf_bad(naffected, npaths, naffected_r, npaths_r, p1, p2):
    return math.log(fn(naffected, npaths, naffected_r, npaths_r, p1, p2)/fn(0, npaths, 0, npaths_r, p1, p2))

# math.log((1.0 - 1.0/k) + p1/((1.0-p2)*k))
def bnf_good(naffected, npaths, naffected_r, npaths_r, p1, p2):
    return math.log((1 - fn(naffected, npaths, naffected_r, npaths_r, p1, p2))/(1 - fn(0, npaths, 0, npaths_r, p1, p2)))

def bnf_weighted(naffected, npaths, naffected_r, npaths_r, p1, p2, weight_good, weight_bad):
    #end_to_end: e2e
    e2e_paths = npaths * npaths_r
    e2e_correct_paths = (npaths - naffected) * (npaths_r - naffected_r)
    e2e_failed_paths = e2e_paths - e2e_correct_paths
    a = float(e2e_failed_paths)/e2e_paths
    val = (1.0 - a) + a * (((1.0 - p1)/p2) ** weight_bad) * ((p1/(1.0-p2)) ** weight_good)
    return math.log((1.0 -a) + a * (((1.0 - p1)/p2) ** weight_bad) * ((p1/(1.0-p2)) ** weight_good))

def bnf_weighted_path_individual(p_arr, correct_p_arr, weight_good, weight_bad):
    likelihood_numerator = 0.0
    likelihood_denominator = 0.0
    #if correct_p_arr[0] > 1.5e-3:
    #    print(p_arr, correct_p_arr)
    for i in range(len(p_arr)):
        p = p_arr[i]
        p0 = correct_p_arr[i]
        likelihood_numerator += (p ** weight_bad) * ((1.0 - p) ** weight_good)
        likelihood_denominator += (p0 ** weight_bad) * ((1.0 - p0) ** weight_good)
    return math.log(likelihood_numerator/likelihood_denominator)
        

def compute_log_likelihood(hypothesis, flows_by_link, flows, min_start_time_ms, max_finish_time_ms, p1, p2):
    log_likelihood = 0.0
    relevant_flows = set()
    for h in hypothesis:
        link_flows = flows_by_link[h]
        for f in link_flows:
            flow = flows[f]
            if flow.start_time_ms >= min_start_time_ms:
                relevant_flows.add(f)

    #an optimization if we know path for every flow
    if PATH_KNOWN:
        weights = [flows[ff].label_weights_func(max_finish_time_ms) for ff in relevant_flows]
        weight_good = sum(w[0] for w in weights)
        weight_bad = sum(w[1] for w in weights)
        #log_likelihood += bnf_bad(1, 1, 1, 1, p1, p2) * weight_bad
        #log_likelihood += bnf_good(1, 1, 1, 1, p1, p2) * weight_good
        #because expression in bnf_weighted runs into numerical issues
        log_likelihood += weight_bad * math.log((1.0 - p1)/p2)
        log_likelihood += weight_good * math.log(p1/(1.0-p2))
        return log_likelihood

    for ff in relevant_flows:
        flow = flows[ff]
        flow_paths = flow.get_paths(max_finish_time_ms)
        weight = flow.label_weights_func(max_finish_time_ms)
        if weight[0] == 0 and weight[1] == 0:
            continue
        npaths = len(flow_paths)
        naffected = 0.0
        p_arr = []
        correct_p_arr = []
        for path in flow_paths:
            pval = 0
            for v in range(1, len(path)):
                l = (path[v-1], path[v])
                if l in hypothesis:
                    naffected += 1.0
                    break
        '''
                    pval += (1.0-p1)
                else:
                    pval += p2
            p_arr.append(pval)
            correct_p_arr.append((len(path)-1) * p2)
        log_likelihood += bnf_weighted_path_individual(p_arr, correct_p_arr, weight[0], weight[1])
        '''
        flow_reverse_paths = flow.get_reverse_paths(max_finish_time_ms)
        npaths_r = len(flow_reverse_paths)
        naffected_r = 0.0
        if CONSIDER_REVERSE_PATH:
            for path in flow_reverse_paths:
                for v in range(1, len(path)):
                    l = (path[v-1], path[v])
                    if l in hypothesis:
                        naffected_r += 1.0
                        break
        #print("Num paths: ", naffected, npaths, naffected_r, npaths_r)
        #log_likelihood += bnf_good(naffected, npaths, naffected_r, npaths_r, p1, p2) * weight[0]
        #log_likelihood += bnf_bad(naffected, npaths, naffected_r, npaths_r, p1, p2) * weight[1]
        log_likelihood += bnf_weighted(naffected, npaths, naffected_r, npaths_r, p1, p2, weight[0], weight[1])
    return log_likelihood
   
def compute_likelihoods(hypothesis_space, flows_by_link, flows, min_start_time_ms, max_finish_time_ms, p1, p2, response_queue):
    start_time = time.time()
    likelihoods = []
    for hypothesis in hypothesis_space:
        log_likelihood = compute_log_likelihood(hypothesis, flows_by_link, flows, min_start_time_ms, max_finish_time_ms, p1, p2)
        #print(log_likelihood, hypothesis)
        likelihoods.append((log_likelihood, list(hypothesis)))
    #print("compute_likelihoods computed", len(hypothesis_space), "in", time.time() - start_time, "seconds") 
    response_queue.put(likelihoods)

def compute_likelihoods_daemon(request_queue, flows_by_link, flows, min_start_time_ms, max_finish_time_ms, p1, p2, response_queue, niters):
    start_time = time.time()
    for i in range(niters):
        hypothesis_space = request_queue.get()
        likelihoods = []
        for hypothesis in hypothesis_space:
            log_likelihood = compute_log_likelihood(hypothesis, flows_by_link, flows, min_start_time_ms, max_finish_time_ms, p1, p2)
            #print(log_likelihood, hypothesis)
            likelihoods.append((log_likelihood, list(hypothesis)))
        #print("compute_likelihoods computed", len(hypothesis_space), "in", time.time() - start_time, "seconds") 
        response_queue.put(likelihoods)


    
def bayesian_network_cilia(flows, links, inverse_links, flows_by_link, forward_flows_by_link, reverse_flows_by_link, failed_links, link_statistics, min_start_time_ms, max_finish_time_ms, params, nprocesses):
    weights = [flow.label_weights_func(max_finish_time_ms) for flow in flows if flow.start_time_ms >= min_start_time_ms]
    weight_good = sum(w[0] for w in weights)
    weight_bad = sum(w[1] for w in weights)
    score_time = time.time()

    #knobs in the bayesian network
    # p1 = P[flow good|bad path], p2 = P[flow bad|good path]
    #p2 = max(1.0e-10, weight_bad/(weight_good + weight_bad))
    p2 = 2e-4
    #p1 = max(0.001, (1.0 - max_alpha_score))
    p1 = 1.0 - 5.0e-3
    if utils.VERBOSE:
        scores, expected_scores = get_link_scores(flows, inverse_links, forward_flows_by_link, reverse_flows_by_link, min_start_time_ms, max_finish_time_ms)
        alpha_scores = [calc_alpha(scores[link], expected_scores[link]) for link in inverse_links]
        max_alpha_score = max(alpha_scores)
        min_expected_flows_on_link = min([expected_scores[link] for link in inverse_links])
        print("Weight (bad):", weight_bad, " (good):", weight_good)
        print("Parameters: P[sample bad|link bad] (1-p1)", 1-p1, "P[sample bad|link good] (p2)", p2)
        print("Min expected coverage on link", min_expected_flows_on_link)
        print("Calculated scores in", time.time() - score_time, " seconds")

    request_queues = []
    response_queue = Queue()
    MAX_FAILS = 10
    for i in range(nprocesses):
        request_queues.append(Queue())
        proc = Process(target=compute_likelihoods_daemon, args=(request_queues[i], dict(flows_by_link), list(flows), min_start_time_ms, max_finish_time_ms, p1, p2, response_queue, MAX_FAILS))
        proc.start()

    n_max_k_likelihoods = 20
    max_k_likelihoods = []
    heappush(max_k_likelihoods, (0.0, []))
    prev_hypothesis_space = [[]]
    NUM_CANDIDATES = min(len(inverse_links), max(15, int(3 * MAX_FAILS)))
    candidates = inverse_links

    if utils.VERBOSE:
        print("Beginning search, num candidates", len(candidates), "branch_factor", len(prev_hypothesis_space))
    start_search_time = time.time()

    for nfails in range(1, MAX_FAILS+1):
        start_time = time.time()
        hypothesis_space = []
        for h in prev_hypothesis_space:
            for link in candidates:
                if link not in h:
                    hnew = sorted(h+[link])
                    if hnew not in hypothesis_space:
                        hypothesis_space.append(hnew)
        num_hypothesis = len(hypothesis_space)
        for i in range(nprocesses):
            start = int(i * num_hypothesis/nprocesses)
            end = int(min(num_hypothesis, (i+1) * num_hypothesis/nprocesses))
            request_queues[i].put(list(hypothesis_space[start:end]))

        l_h = [] # each element is (likelihood, hypothesis)
        for i in range(nprocesses):
            l_h.extend(response_queue.get())
        top_hypotheses = np.argsort([x[0] for x in l_h])
        for i in range(min(n_max_k_likelihoods, len(top_hypotheses))):
            ind = top_hypotheses[-(i+1)]
            heappush(max_k_likelihoods, l_h[ind])
            if (len(max_k_likelihoods) > n_max_k_likelihoods):
                heappop(max_k_likelihoods)

        if nfails == 1:
            candidates_likelihoods = [l_h[i] for i in top_hypotheses[-NUM_CANDIDATES:]]
            candidates = [l_h[1][0] for l_h in candidates_likelihoods] #update candidates for further search
            if utils.VERBOSE:
                for l, h in candidates_likelihoods:
                    link = h[0]
                    print(link, l, calc_alpha(scores[link], expected_scores[link]), scores[link], expected_scores[link])
                for l,h in l_h:
                    link = h[0]
                    if link in failed_links:
                        print("Failed link: ", link, l, calc_alpha(scores[link], expected_scores[link]), " scores: ", scores[link], expected_scores[link])

        #Aggressively limit candidates at further stages
        NUM_CANDIDATES = min(len(inverse_links), 10)
        if utils.VERBOSE:
            print("Finished hypothesis search across ", len(hypothesis_space), "Hypothesis for ", nfails, "failures in", time.time() - start_time, "seconds")
        prev_hypothesis_space = [l_h[i][1] for i in top_hypotheses[-NUM_CANDIDATES:]]

    failed_links_set = set(failed_links.keys())
    if utils.VERBOSE:
        likelihood_correct_hypothesis = compute_log_likelihood(failed_links_set, flows_by_link, flows, min_start_time_ms, max_finish_time_ms, p1, p2)
        print ("Correct Hypothesis ", list(failed_links_set), " likelihood ", likelihood_correct_hypothesis)
    highest_likelihood = -1000000000000.0 
    for likelihood, hypothesis in max_k_likelihoods:
        highest_likelihood = max(likelihood, highest_likelihood)
    ret_hypothesis = set()
    while(len(max_k_likelihoods) > 0):
        likelihood, hypothesis = heappop(max_k_likelihoods)
        if(highest_likelihood > 1.0e-3 and highest_likelihood - likelihood <= 1.0e-3):
            for h in hypothesis:
                ret_hypothesis.add(h)
        if utils.VERBOSE:
            print ("Likely candidate", hypothesis, likelihood)
    precision, recall = get_precision_recall(failed_links, ret_hypothesis)
    if utils.VERBOSE:
        print("\nSearched hypothesis space in ", time.time() - start_search_time, " seconds")
        print ("Output Hypothesis: ", list(ret_hypothesis))
    return (precision, recall), None


