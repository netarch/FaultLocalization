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

    
def net_bouncer(flows, links, inverse_links, flows_by_link, forward_flows_by_link, reverse_flows_by_link, failed_links, link_statistics, min_start_time_ms, max_finish_time_ms, params, nprocesses):
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
    def opt (lx):
        c = 0.002
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
        if X[i] <= 0.9995:
            print(inverse_links[i], X[i])
            ret_hypothesis.append(inverse_links[i])
    precision, recall = get_precision_recall(failed_links, ret_hypothesis)
    if utils.VERBOSE:
        print ("Output Hypothesis: ", list(ret_hypothesis))
    return (precision, recall), None


