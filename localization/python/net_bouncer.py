import sys
from heapq import heappush, heappop
from multiprocessing import Process, Queue
import math
import time
import random
import numpy as np
from scipy.optimize import minimize
import utils
from utils import *
from plot_utils import *

def net_bouncer(flows, links, inverse_links, flows_by_link, forward_flows_by_link, reverse_flows_by_link, failed_links, link_statistics, min_start_time_ms, max_finish_time_ms, params, nprocesses):
    start_time = time.time()
    nlinks = len(inverse_links)
    if utils.VERBOSE:
        print("Num links", nlinks)
    sum_success_prob = np.zeros(nlinks)
    num_flows_through_link = np.zeros(nlinks)
    filtered_flows = []
    for flow in flows:
        if flow.start_time_ms >= min_start_time_ms and flow.any_snapshot_before(max_finish_time_ms) and flow.traceroute_flow(max_finish_time_ms):
            filtered_flows.append(flow)
            path = flow.path_taken
            for v in range(1, len(path)):
                l = (path[v-1], path[v])
                sum_success_prob[links[l]] += (1.0 - flow.get_drop_rate(max_finish_time_ms))
                num_flows_through_link[links[l]] += 1
    #X: success probabilities
    flows = filtered_flows
    flows_by_link = get_forward_flows_by_link(flows, inverse_links, max_finish_time_ms)
    X = np.zeros(nlinks)
    for i in range(nlinks):
        X[i] = sum_success_prob[i]/num_flows_through_link[i]
        
    if utils.VERBOSE:
        print("Initial X constructed in", time.time() - start_time, "seconds")

    c = params[0]
    threshold = params[1]
    def F(X):
        ret = 0.0
        for flow in flows:
            if flow.start_time_ms >= min_start_time_ms and flow.any_snapshot_before(max_finish_time_ms) and flow.traceroute_flow(max_finish_time_ms):
                path = flow.path_taken
                x = 1.0
                for v in range(1, len(path)):
                    l = (path[v-1], path[v])
                    x *= X[links[l]]
                y = 1.0 - flow.get_drop_rate(max_finish_time_ms)
                ret += (y-x)*(y-x)
        for i in range(nlinks):
            ret += c * X[i] * (1.0 - X[i])
        return ret
        
    def argmin_F_xi(X, i):
        S1 = 0.0
        S2 = 0.0
        for ff in flows_by_link[inverse_links[i]]:
            flow = flows[ff]
            if flow.start_time_ms >= min_start_time_ms and flow.any_snapshot_before(max_finish_time_ms) and flow.traceroute_flow(max_finish_time_ms):
                path = flow.path_taken
                contains_xi = False
                t = 1.0
                for v in range(1, len(path)):
                    l = (path[v-1], path[v])
                    if links[l] == i:
                        contains_xi = True
                    else:
                        t *= X[links[l]]
                assert(contains_xi)
                if contains_xi:
                    y = 1.0 - flow.get_drop_rate(max_finish_time_ms)
                    S1 += t * y
                    S2 += t * t
        return (2 * S1 - c)/(2 * (S2 - c))

    maxiter = 50
    error = F(X)
    for it in range(maxiter):
        iter_start_time = time.time()
        X_new = np.zeros(nlinks)
        for i in range(nlinks):
            X_new[i] = max(0.0, min(argmin_F_xi(X, i), 1.0))
        X = X_new
        new_error = F(X)
        error_diff = error - new_error
        if utils.VERBOSE:
            print("Iteration", it, "Error", new_error, "finished in", time.time()-iter_start_time)
        if abs(error_diff) < 1.0e-6:
            break
        error = new_error

    ret_hypothesis = []
    #print(np.sort(X))
    for i in range(nlinks):
        if X[i] <= 1-((1-threshold)/2.0):
            print(inverse_links[i], X[i])
        if X[i] <= threshold:
            ret_hypothesis.append(inverse_links[i])
    precision, recall = get_precision_recall(failed_links, ret_hypothesis)
    if utils.VERBOSE:
        print ("Output Hypothesis: ", list(ret_hypothesis), "Time taken", time.time() - start_time, "seconds")
    return (precision, recall), None

    
def net_bouncer2(flows, links, inverse_links, flows_by_link, forward_flows_by_link, reverse_flows_by_link, failed_links, link_statistics, min_start_time_ms, max_finish_time_ms, params, nprocesses):
    start_time = time.time()
    nequations = sum([1 for flow in flows if flow.start_time_ms >= min_start_time_ms and flow.any_snapshot_before(max_finish_time_ms)])
    nlinks = len(inverse_links)
    LA = np.zeros(shape=(nequations, nlinks))
    LY = np.zeros(nequations)
    if utils.VERBOSE:
        print("Num equations", nequations, "Num links", nlinks)
    ff = 0
    for flow in flows:
        if flow.start_time_ms >= min_start_time_ms and flow.any_snapshot_before(max_finish_time_ms):
            path = flow.path_taken
            for v in range(1, len(path)):
                l = (path[v-1], path[v])
                LA[ff][links[l]] = 1.0
            LY[ff] = math.log(1.0 - flow.get_drop_rate(max_finish_time_ms))
            ff += 1
    if utils.VERBOSE:
        print("LA LY constructed in", time.time() - start_time, "seconds")

    Y = np.power(math.e, LY)
    c = params[0]
    threshold = params[1]
    def opt (lx):
        #c = 0.005
        lX = np.dot(LA, lx)
        X = np.power(math.e, lX)
        norm = np.linalg.norm(X-Y)
        return norm * norm + c * (sum(X) - sum(np.power(X, 2.0))) 

    sol = minimize(opt, np.zeros(nlinks), method='L-BFGS-B', bounds=[(-0.2,0.0) for x in range(nlinks)])
    lX = sol['x']
    X = np.power(math.e, lX)
    ret_hypothesis = []
    #print(np.sort(X))
    for i in range(nlinks):
        if X[i] <= 1-((1-threshold)/2.0):
            print(inverse_links[i], X[i])
        if X[i] <= threshold:
            ret_hypothesis.append(inverse_links[i])
    precision, recall = get_precision_recall(failed_links, ret_hypothesis)
    if utils.VERBOSE:
        print ("Output Hypothesis: ", list(ret_hypothesis))
    return (precision, recall), None


