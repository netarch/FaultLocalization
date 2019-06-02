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
from common_testing_system import *
from common_bayesian_net import *
from net_bouncer import *

def get_files():
    file_prefix = "../topology/net_bouner_ft_k8_os3/logs/plog_1"
    files = []
    for i in range(1,9):
        #for l in ["0.01", "0.005", "0.002", "0.001"]:
        for l in ["0.01"]:
            files.append(file_prefix + "_" + l + "_" + str(i))
    '''
    for i in range(1,25):
        l="0"
        files.append(file_prefix + "_" + l + "_" + str(i))
    '''
    return files

def get_files1():
    files = []
    file_prefix = "../topology/net_bouner_ft_k8_os3/logs/plog_0"
    for i in range(1,25):
        files.append(file_prefix + "_0_" + str(i))
    for f in range(1,9):
        file_prefix = "../topology/net_bouner_ft_k8_os3/logs/plog_" + str(f)
        for s in range(1,13):
            files.append(file_prefix +"_0_" + str(s))
    return files

def get_files2():
    files = ["../topology/ls_x45_y15/logs/plog_1_0_1"]
    #files = ["../plog"]
    return files

def get_precision_recall_trend_estimator(min_start_time_ms, max_finish_time_ms, estimator_func, params, nprocesses):
    files = get_files2()
    for f in files:
        print("File: ", f)
    step = (max_finish_time_ms - min_start_time_ms)/10
    step = 200.0
    precision_recall, info = get_precision_recall_trend(files, min_start_time_ms, max_finish_time_ms, step, estimator_func, params, nprocesses)
    for i in range(len(precision_recall)):
        p, r = precision_recall[i]
        print("Max_finish_time_ms: ", min_start_time_ms + i*step, p, r)

def get_precision_recall_trend_bayesian_cilia(min_start_time_ms, max_finish_time_ms, nprocesses):
    get_precision_recall_trend_estimator(min_start_time_ms, max_finish_time_ms, bayesian_network_cilia, [], nprocesses)


def get_precision_recall_trend_net_bouncer(min_start_time_ms, max_finish_time_ms, nprocesses):
    get_precision_recall_trend_estimator(min_start_time_ms, max_finish_time_ms, net_bouncer, [], nprocesses)




if __name__ == '__main__':
    min_start_time_sec = float(sys.argv[1])
    max_finish_time_sec = float(sys.argv[2])
    nprocesses = 30
    utils.VERBOSE = False
    #get_precision_recall_trend(file_prefix, minfile, maxfile, [nfails], nprocesses)
    start_time = time.time()
    get_precision_recall_trend_bayesian_cilia(min_start_time_sec * 1000.0, max_finish_time_sec * 1000.0, nprocesses)
    #get_precision_recall_trend_net_bouncer(min_start_time_sec * 1000.0, max_finish_time_sec * 1000.0, nprocesses)
    print("Execution time", time.time() - start_time, "seconds")

