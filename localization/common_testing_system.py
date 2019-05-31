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
    #print("Time to partition flows by link: ", time.time() - start_time_ms, "seconds")
    flows, links, inverse_links, flows_by_link, forward_flows_by_link, reverse_flows_by_link, link_statistics, failed_links = get_data_structures_from_logfile(filename)
    if utils.VERBOSE:
        numflows = sum([1 for flow in flows if flow.start_time_ms >= min_start_time_ms and flow.finish_time_ms<= max_finish_time_ms])
        active_probes = sum([1 for flow in flows if flow.start_time_ms >= min_start_time_ms and flow.finish_time_ms<= max_finish_time_ms and flow.traceroute_flow])
        print("Num flows", numflows)
        print("Active probes", active_probes)
    return estimator_func(flows, links, inverse_links, flows_by_link, forward_flows_by_link, reverse_flows_by_link, failed_links, link_statistics, min_start_time_ms, max_finish_time_ms, params, nprocesses)
   
def get_precision_recall_trend_file(filename, min_start_time_ms, max_finish_time_ms, step, estimator_func, params):
    flows, links, inverse_links, flows_by_link, forward_flows_by_link, reverse_flows_by_link, link_statistics, failed_links = get_data_structures_from_logfile(filename)
    precision_recalls = []
    last_print_time = min_start_time_ms
    retinfo = []
    for finish_time_ms in np.arange(min_start_time_ms, max_finish_time_ms, step):
        if (finish_time_ms - last_print_time >= 1 * step):
            last_print_time = min_start_time_ms
            #active_flows = sum([1.0 for flow in flows if flow.traceroute_flow and flow.start_time_ms >= min_start_time_ms and flow.finish_time_ms <= max_finish_time_ms])
            print(filename, finish_time_ms)
        #!TODO: HACK HACK for ns3 logs
        f_t = int(finish_time_ms/1000.0)
        s_t = f_t - 1
        p_r, info = estimator_func(flows, links, inverse_links, flows_by_link, forward_flows_by_link, reverse_flows_by_link, failed_links, link_statistics, s_t*1000.0, f_t*1000.0, params, 1)
        p, r = p_r
        #print(p_r, info, s_t, f_t):w
        if finish_time_ms + step >= max_finish_time_ms:
            print(filename, p, r)
        if utils.VERBOSE:
            print("get_precision_recall_trend_file", finish_time_ms, p, r, min_start_time_ms)
        precision_recalls.append((p, r))
        if info != None:
            retinfo.append(info)
    return precision_recalls, retinfo
    
    
def get_precision_recall_trend_files_serial(files, min_start_time_ms, max_finish_time_ms, step, estimator_func, params, response_queue):
    ret = []
    for filename in files:
        pr_trend, info = get_precision_recall_trend_file(filename, min_start_time_ms, max_finish_time_ms, step, estimator_func, params)
        ret.append((filename, pr_trend, info))
        #print(file_prefix, pr_trend) 
    response_queue.put(ret)
        
    
def get_precision_recall_trend_files_parallel(files, min_start_time_ms, max_finish_time_ms, step, estimator_func, params, nprocesses):
    start_time_ms = time.time()
    procs = []
    files_copies = []
    numfiles = len(files)
    for i in range(nprocesses):
        start = int(i * numfiles/nprocesses)
        end = int(min(numfiles, (i+1) * numfiles/nprocesses))
        print("Chunk assigned to process: ", start, end)
        files_copies.append(list(files[start:end]))
    print("Arrays copied for parallel execution in ", time.time() - start_time_ms, " seconds")
    response_queue = Queue()
    for i in range(nprocesses):
        proc = Process(target=get_precision_recall_trend_files_serial, args=(files_copies[i], min_start_time_ms, max_finish_time_ms, step, estimator_func, params, response_queue))
        procs.append(proc)
    for proc in procs:
        proc.start()
    #for proc in procs:
    #    proc.join()
    precision_recall = []
    retinfo = []
    for finish_time in np.arange(min_start_time_ms, max_finish_time_ms, step):
        precision_recall.append((0.0, 0.0))

    for i in range(nprocesses):
        response = response_queue.get()
        for filename, p_r_list, info in response:
            if info!=None:
                retinfo.append(info)
            for j in range(len(p_r_list)):
                p1, r1 = p_r_list[j]
                p2, r2 = precision_recall[j]
                precision_recall[j] = (p1+p2, r1+r2)
    print("Finished sweeping all epochs in ", time.time() - start_time_ms, "seconds")
    for i in range(len(p_r_list)):
        p, r = precision_recall[i]
        p /= len(files)
        r /= len(files)
        precision_recall[i] = (p, r)
    return precision_recall, retinfo

def get_precision_recall_trend(files, min_start_time_ms, max_finish_time_ms, step, estimator_func, params, nprocesses):
    nprocesses = min(len(files), nprocesses)
    precision_recall, info = get_precision_recall_trend_files_parallel(files, min_start_time_ms, max_finish_time_ms, step, estimator_func, params, nprocesses)
    return precision_recall, info

