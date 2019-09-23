import sys
from heapq import heappush, heappop
from multiprocessing import Process, Queue
import pyximport; pyximport.install()
import math
import time
import random
import numpy as np
from scipy.optimize import minimize
import utils
from utils import *


def compute_max_traceroute(badflows):
    def add_tr_for_path(num_trs, path):
        for v in path:
            if v not in num_trs:
                num_trs[v] = 0
            num_trs[v] += 1
    num_trs = dict()
    for flow in badflows:
        add_tr_for_path(num_trs, flow.path_taken)
        add_tr_for_path(num_trs, flow.reverse_path_taken)
    if len(num_trs) > 0:
        return max(num_trs.values())
    else:
        return 0

def doubleO7(flows, links, inverse_links, flows_by_link, forward_flows_by_link, reverse_flows_by_link, failed_links, link_statistics, min_start_time_ms, max_finish_time_ms, params, nprocesses):
    start_time = time.time()
    fail_percentile = params[0] 
    badflows = [flow for flow in flows if flow.start_time_ms >= min_start_time_ms and flow.label_weights_func(max_finish_time_ms)[1]>0]
    numbadflows = len(badflows)
    num_trs = compute_max_traceroute(badflows)
    #if num_trs > 250:
    #    print("Numbadflows: ", numbadflows, " max traceroute/switch: ", num_trs)
    problematic_links = []
    votes = dict()
    def compute_votes(badflows):
        filtered_badflows = []
        for link in inverse_links:
            votes[link] = 0.0
        discarded_flows = 0.0
        sumvotes = 0.0
        for flow in badflows:
            flow_paths = flow.get_paths(max_finish_time_ms)
            assert(len(flow_paths) == 1)
            discard_flow = False
            numlinks = 0
            for path in flow_paths:
                for v in range(1, len(path)):
                    l = (path[v-1], path[v])
                    if l in problematic_links:
                        discard_flow = True
                        break
                    numlinks += 1
            if (not discard_flow) and CONSIDER_REVERSE_PATH:
                flow_reverse_paths = flow.get_reverse_paths(max_finish_time_ms)
                assert(len(flow_paths) == 1)
                for path in flow_reverse_paths:
                    for v in range(1, len(path)):
                        l = (path[v-1], path[v])
                        if l in problematic_links:
                            discard_flow = True
                            break
                        numlinks += 1
            if not discard_flow:
                filtered_badflows.append(flow)
                flowvote = float(flow.label_weights_func(max_finish_time_ms)[1])
                vote = flowvote/numlinks
                sumvotes += flowvote
                for path in flow_paths:
                    for v in range(1, len(path)):
                        l = (path[v-1], path[v])
                        votes[l] += vote
                if CONSIDER_REVERSE_PATH:
                    flow_reverse_paths = flow.get_reverse_paths()
                    for path in flow_reverse_paths:
                        for v in range(1, len(path)):
                            l = (path[v-1], path[v])
                            votes[l] += vote
            else:
                discarded_flows += 1
        assert(discarded_flows > 0 or len(problematic_links)==0)
        return (filtered_badflows, max(votes.values()), sumvotes)

    badflows, maxvotes, sumvotes = compute_votes(badflows)
    while (maxvotes >= fail_percentile * sumvotes and maxvotes>0):
        faulty_link = max(votes, key=lambda k: votes[k])
        if utils.VERBOSE:
            print("Faulty link", faulty_link, "votes", votes[faulty_link], "sumvotes", sumvotes)
        problematic_links.append(faulty_link)
        badflows, maxvotes, sumvotes = compute_votes(badflows)

    if utils.VERBOSE:
    	print("007 inference finished in", time.time() - start_time, "second")
    return get_precision_recall(failed_links, problematic_links), None

