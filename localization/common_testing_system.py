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


def get_precision_recall_estimator(filename, min_start_time_ms, max_finish_time_ms, estimator_func, params, nprocesses):
    #print("Time to partition flows by link: ", time.time() - start_time, "seconds")
    flows, inverse_links, flows_by_link, forward_flows_by_link, reverse_flows_by_link, link_statistics, failed_links = get_data_structures_from_logfile(filename)
    active_probes = sum([1.0 for flow in flows if flow.traceroute_flow])
    if utils.VERBOSE:
        print("Active probes", active_probes)
    return estimator_func(flows, inverse_links, flows_by_link, forward_flows_by_link, reverse_flows_by_link, failed_links, link_statistics, min_start_time_ms, max_finish_time_ms, params, nprocesses)
   
def get_precision_recall_trend_file(filename, estimator_func, min_start_time, max_finish_time, step):
    flows, inverse_links, flows_by_link, forward_flows_by_link, reverse_flows_by_link, link_statistics, failed_links = get_data_structures_from_logfile(filename)
    precision_recalls = []
    last_print_time = min_start_time
    for finish_time in range(min_start_time, max_finish_time, step):
        precision_recall.append((0.0, 0.0))
        if (finish_time - last_print_time >= 5 * step):
            last_print_time = min_start_time
            active_flows = sum([1.0 for flow in flows if flow.traceroute_flow and flow.start_time >= min_start_time and flow.epoch <= max_finish_time])
            print(filename, finish_time, "active_flows", active_flows)
        p, r = estimator_func(flows, inverse_links, flows_by_link, forward_flows_by_link, reverse_flows_by_link, failed_links, link_statistics, min_start_time, finish_time, None, 1)
        if epoch == maxfile:
            print(file_prefix, p, r)
        if utils.VERBOSE:
            print("get_precision_recall_trend_file", epoch, p, r, minfile, epoch)
        precision_recalls.append((p, r))
    return precision_recalls
    
    
def get_precision_recall_trend_files_serial(files, estimator_func, min_start_time, max_finish_time, step, response_queue):
    ret = []
    for filename in files:
        pr_trend, info = get_precision_recall_trend_file(filename, estimator_func, min_start_time, max_finish_time, step)
        ret.append((filename, pr_trend, info))
        #print(file_prefix, pr_trend) 
    response_queue.put(ret)
        
    
def get_precision_recall_trend_files_parallel(files, estimator_func, min_start_time, max_finish_time, step, nprocesses):
    start_time = time.time()
    procs = []
    files_copies = []
    numfiles = len(files)
    for i in range(nprocesses):
        start = int(i * numfiles/nprocesses)
        end = int(min(numfiles, (i+1) * numfiles/nprocesses))
        print("Chunk assigned to process: ", start, end)
        files_copies.append(list(files[start:end]))
    print("Arrays copied for parallel execution in ", time.time() - start_time, " seconds")
    response_queue = Queue()
    for i in range(nprocesses):
        proc = Process(target=get_precision_recall_trend_files_serial, args=(files[i], estimator_func, min_start_time, max_finish_time, step, response_queue))
        procs.append(proc)
    for proc in procs:
        proc.start()

    #for proc in procs:
    #    proc.join()
 
    precision_recall = []
    for epoch in range(min_start_time, max_finish_time, step):
        precision_recall.append((0.0, 0.0))

    for i in range(nprocesses):
        response = response_queue.get()
        for filename, p_r_list, info in response:
            for j in range(len(p_r_list)):
                p1, r1 = p_r_list[j]
                p2, r2 = precision_recall[j]
                precision_recall[j] = (p1+p2, r1+r2)
    print("Finished sweeping all epochs in ", time.time() - start_time, "seconds")
    for i in range(len(p_r_list)):
        p, r = precision_recall[i]
        p /= len(files)
        r /= len(files)
        precision_recall[i] = (p, r)
    return precision_recall

def get_precision_recall_trend(files, estimator_func, min_start_time, max_finish_time, step, nprocesses):
    for f in files:
        print("File: ", f)
    nprocesses = min(len(files), nprocesses)
    precision_recall, info = get_precision_recall_trend_files_parallel(files, estimator_func, min_start_time, max_finish_time, step, nprocesses)
    for i in range(len(precision_recall)):
        p, r = precision_recall[i]
        print("Max_finish_time: ", min_start_time + i*step, p, r)

