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
from common_testing_system import *
from bayesian_net import *
from net_bouncer import *
from doubleO7 import *

def get_files():
    file_prefix = "../topology/net_bouncer_ft_k8_os2/logs/active_passive/plog"
    #file_prefix = "../topology/net_bouncer_ft_k8_os2/logs/new/plog"
    #file_prefix = "../topology/net_bouncer_ft_k8_os2/logs/plog"
    ignore_files = []
    files = []
    for f in range(1,9):
        #for s in [3,4,5,6]:
        for s in [1,2]: #ap
        #for s in [3,4,5,6]:
            if (f, s) not in ignore_files:
                files.append(file_prefix + "_" + str(f) + "_0_" + str(s))
    #for s in range(1,17):
    #    files.append(file_prefix + "_0_0_" + str(s))
    return files

def get_files1():
    ignore_files = []
    #ignore_files_old = [(6,3), (6,1), (7,1)]
    #ignore_files += [(0,7),(0,8),(6,3),(8,1),(2,2),(6,14)]
    files = []
    file_prefix = "../topology/ls_x30_y10/logs/plog_0"
    #inds = [9,12,15,16,20,21,22,23]
    for i in range(1,1):
        if (0, i) not in ignore_files:
            files.append(file_prefix + "_0_" + str(i))
    for f in range(1,9):
        file_prefix = "../topology/ls_x30_y10/logs/plog_" + str(f)
        for s in [1,2,4]:
            if (f, s) not in ignore_files:
                files.append(file_prefix +"_0_" + str(s))
    return files

def get_files2():
    #files = ["../topology/ls_x45_y15/logs/plog_1_0_1"]
    files = ["../plog"]
    return files

def get_precision_recall_trend_estimator(min_start_time_ms, max_finish_time_ms, estimator_func, params, nprocesses):
    files = get_files1()
    for f in files:
        print("File: ", f)
    step = (max_finish_time_ms - min_start_time_ms)/10
    step = 1.0 * 1000.0
    precision_recall, info = get_precision_recall_trend(files, min_start_time_ms, max_finish_time_ms, step, estimator_func, params, nprocesses)
    for i in range(len(precision_recall)):
        p, r = precision_recall[i]
        print("Max_finish_time_ms: ", min_start_time_ms + (i+1)*step, p, r)

def get_precision_recall_trend_bayesian_cilia(min_start_time_ms, max_finish_time_ms, nprocesses):
    #(1.0 - 2e-3, 1.2e-4 works well)
    #get_precision_recall_trend_estimator(min_start_time_ms, max_finish_time_ms, bayesian_network_cilia, (1.0 - 6e-3, 7.5e-4), nprocesses)
    get_precision_recall_trend_estimator(min_start_time_ms, max_finish_time_ms, bayesian_network_cilia, (1.0 - 2.5e-3, 1.0e-4), nprocesses)

def get_precision_recall_trend_net_bouncer(min_start_time_ms, max_finish_time_ms, nprocesses):
    #get_precision_recall_trend_estimator(min_start_time_ms, max_finish_time_ms, net_bouncer, (0.0075, 1-3.0e-3), nprocesses)
    get_precision_recall_trend_estimator(min_start_time_ms, max_finish_time_ms, net_bouncer, (0.0125, 1-6e-3), nprocesses)

def get_precision_recall_trend_007(min_start_time_ms, max_finish_time_ms, fail_percentile, nprocesses):
    get_precision_recall_trend_estimator(min_start_time_ms, max_finish_time_ms, doubleO7, (fail_percentile,), nprocesses)

def get_precision_recall_bayesian_cilia_params(min_start_time_ms, max_finish_time_ms, nprocesses):
    files = get_files1()
    p1 = list(np.arange(0.001, 0.003, 0.0005))
    p1 = [1.0 - x for x in p1]
    p2 = list(np.arange(0.000050, 0.00035, 0.00005))
    params_list = []
    for p1_i in p1:
        for p2_i in p2:
            params_list.append((p1_i, p2_i))
    nprocesses = min(len(files), nprocesses)
    precision_recall = explore_params_estimator_files(files, min_start_time_ms, max_finish_time_ms, bayesian_network_cilia, params_list, nprocesses)
    for params in params_list:
        print(params, precision_recall[params])

def get_precision_recall_net_bouncer_params(min_start_time_ms, max_finish_time_ms, nprocesses):
    files = get_files()
    #c = [0.00625]
    #c = [0.00750, 0.00875, 0.01, 0.01125, 0.0125, 0.015]
    c = np.arange(0.00250, 0.0175, 0.00125)
    #threshold = [2.0e-3, 2.5e-3, 3e-3, 3.5e-3, 4e-3]
    threshold = [5.5e-3, 6e-3]
    threshold = [1.0 - x for x in threshold]
    params_list = []
    for c_i in c:
        for t_i in threshold:
            params_list.append((c_i, t_i))
    nprocesses = min(len(files), nprocesses)
    precision_recall = explore_params_estimator_files(files, min_start_time_ms, max_finish_time_ms, net_bouncer, params_list, nprocesses)
    for params in params_list:
        print(params, precision_recall[params])

def get_precision_recall_007_params(min_start_time_ms, max_finish_time_ms, nprocesses):
    files = get_files1()
    fail_percentiles = list(np.arange(0.001, 0.03, 0.0025))
    print(fail_percentiles)
    params_list = [(x,) for x in fail_percentiles]
    nprocesses = min(len(files), nprocesses)
    precision_recall = explore_params_estimator_files(files, min_start_time_ms, max_finish_time_ms, doubleO7, params_list, nprocesses)
    for params in params_list:
        print(params, precision_recall[params])

if __name__ == '__main__':
    min_start_time_sec = float(sys.argv[1])
    max_finish_time_sec = float(sys.argv[2])
    nprocesses = 16
    utils.VERBOSE = False
    start_time = time.time()
    get_precision_recall_trend_bayesian_cilia(min_start_time_sec * 1000.0, max_finish_time_sec * 1000.0, nprocesses)
    #print("Execution time", time.time() - start_time, "seconds")
    #get_precision_recall_trend_net_bouncer(min_start_time_sec * 1000.0, max_finish_time_sec * 1000.0, nprocesses)
    #fail_percentile = 0.0135
    #get_precision_recall_trend_007(min_start_time_sec * 1000.0, max_finish_time_sec * 1000.0, fail_percentile, nprocesses)
    #get_precision_recall_net_bouncer_params(min_start_time_sec * 1000.0, max_finish_time_sec * 1000.0, nprocesses)
    print("Execution time", time.time() - start_time, "seconds")

