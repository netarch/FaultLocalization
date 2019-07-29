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
from doubleO7 import *
from net_bouncer import *


def get_precision_recall_bayesian_cilia(filename, min_start_time_ms, max_finish_time_ms, nprocesses):
    return get_precision_recall_estimator(filename, min_start_time_ms, max_finish_time_ms, bayesian_network_cilia, (1.0 - 2.5e-3, 2.5e-4), nprocesses)
    #return get_precision_recall_estimator(filename, min_start_time_ms, max_finish_time_ms, bayesian_network_cilia, (1.0 - 2.5e-3, 0.5e-4), nprocesses)

def get_precision_recall_net_bouncer(filename, min_start_time_ms, max_finish_time_ms, nprocesses):
    return get_precision_recall_estimator(filename, min_start_time_ms, max_finish_time_ms, net_bouncer, (0.0075, 1-3.0e-3), nprocesses)
   
def get_precision_recall_007(filename, min_start_time_ms, max_finish_time_ms, fail_percentile, nprocesses):
    return get_precision_recall_estimator(filename, min_start_time_ms, max_finish_time_ms, doubleO7, [fail_percentile], nprocesses)
   

if __name__ == '__main__':
    filename = sys.argv[1]
    min_start_time_sec = float(sys.argv[2])
    max_finish_time_sec = float(sys.argv[3])
    nprocesses = 1
    utils.VERBOSE = True
    start_time = time.time()
    precision_recall, info = get_precision_recall_bayesian_cilia(filename, min_start_time_sec * 1000.0, max_finish_time_sec * 1000.0, nprocesses)
    #precision_recall, info = get_precision_recall_net_bouncer(filename, min_start_time_sec * 1000.0, max_finish_time_sec * 1000.0, nprocesses)
    #fail_percentile = float(sys.argv[4])
    #fail_percentile = 0.016
    #precision_recall, info = get_precision_recall_007(filename, min_start_time_sec * 1000.0, max_finish_time_sec * 1000.0, fail_percentile, nprocesses)
    print(filename, "precision recall", precision_recall, "execution time", time.time() - start_time, "seconds")

